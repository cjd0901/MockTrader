mod parse;
mod range;

use std::path::{Path, PathBuf};

use anyhow::Context;
use tokio::fs;
use tokio::io::{AsyncReadExt, AsyncSeekExt};

use crate::indicators::{self, compute_from_ohlc, IndicatorSeries};
use crate::api::StockEntry;
use crate::protocol::KLINE_RECORD_SIZE;

use parse::{ohlc_from_records, record_close, unix_ts_from_records};
use range::{file_unix_range, find_first_index_ge, find_last_index_le};

const RECORD_SIZE: u64 = KLINE_RECORD_SIZE as u64;
/// Extra bars before the requested window for MACD/KDJ warmup.
const INDICATOR_WARMUP_BARS: u64 = 150;

#[derive(Clone)]
pub struct KlineStore {
    dir: PathBuf,
}

impl KlineStore {
    pub fn new(dir: PathBuf) -> Self {
        Self { dir }
    }

    pub async fn list_stocks(&self) -> anyhow::Result<Vec<StockEntry>> {
        let mut out = Vec::new();
        let mut rd = fs::read_dir(&self.dir)
            .await
            .with_context(|| format!("read kline dir {}", self.dir.display()))?;

        while let Some(ent) = rd.next_entry().await? {
            let path = ent.path();
            if path.extension().and_then(|s| s.to_str()) != Some("bin") {
                continue;
            }
            if let Some(stock) = parse_stock_from_filename(path.file_name().and_then(|n| n.to_str())) {
                out.push(stock);
            }
        }

        out.sort_by(|a, b| a.symbol.cmp(&b.symbol));
        Ok(out)
    }

    /// Returns `(start_index, total_bars, raw_record_bytes, indicator_bytes)`.
    pub async fn read_candles_raw(
        &self,
        symbol: &str,
        before_index: Option<u64>,
        limit: u32,
    ) -> anyhow::Result<(u64, u64, Vec<u8>, Vec<u8>)> {
        let path = self.resolve_path(symbol).await?;
        let meta = fs::metadata(&path).await?;
        let total = meta.len() / RECORD_SIZE;
        if total == 0 {
            return Ok((0, 0, Vec::new(), Vec::new()));
        }

        let limit = limit.min(20_000) as u64;
        let end = before_index.unwrap_or(total).min(total);
        let start = end.saturating_sub(limit);
        let warmup_start = start.saturating_sub(INDICATOR_WARMUP_BARS);
        let warmup_count = end - warmup_start;

        let warmup_buf = read_record_bytes(&path, warmup_start, warmup_count).await?;

        let slice_off = ((start - warmup_start) * RECORD_SIZE) as usize;
        let records = warmup_buf[slice_off..].to_vec();

        let (closes, highs, lows) = ohlc_from_records(&warmup_buf);
        let series = compute_from_ohlc(&closes, &highs, &lows);
        let offset = (start - warmup_start) as usize;
        let end = offset + records.len() / KLINE_RECORD_SIZE;
        let indicators = indicators::encode_wire_range(&series, offset, end);

        debug_assert_eq!(
            records.len() / KLINE_RECORD_SIZE,
            indicators.len() / indicators::INDICATOR_VALUES_SIZE
        );

        Ok((start, total, records, indicators))
    }

    /// Unix timestamps of the first and last bar in the symbol file.
    pub async fn file_time_range(&self, symbol: &str) -> anyhow::Result<(i64, i64, u64)> {
        let path = self.resolve_path(symbol).await?;
        let meta = fs::metadata(&path).await?;
        let total = meta.len() / RECORD_SIZE;
        if total == 0 {
            anyhow::bail!("empty kline file for {symbol}");
        }
        let (min_ts, max_ts) = file_unix_range(&path, total).await?;
        Ok((min_ts, max_ts, total))
    }

    /// Run backtest on `[start_ts, end_ts]` (unix seconds, inclusive) using binary search on the file.
    pub async fn run_backtest(
        &self,
        catalog: &crate::strategy::StrategyCatalog,
        symbol: &str,
        strategy_id: &str,
        start_ts: i64,
        end_ts: i64,
    ) -> anyhow::Result<crate::strategy::BacktestResult> {
        if start_ts > end_ts {
            anyhow::bail!("startTs must be <= endTs");
        }

        let path = self.resolve_path(symbol).await?;
        let meta = tokio::fs::metadata(&path).await?;
        let total = meta.len() / RECORD_SIZE;
        if total == 0 {
            return Ok(crate::strategy::empty_backtest_result());
        }

        let (file_min_ts, file_max_ts) = file_unix_range(&path, total).await?;
        let start_ts = start_ts.clamp(file_min_ts, file_max_ts);
        let end_ts = end_ts.clamp(file_min_ts, file_max_ts);

        let lo = find_first_index_ge(&path, total, start_ts).await?;
        let mut hi = find_last_index_le(&path, total, end_ts).await?;
        if lo > hi || lo >= total {
            return Ok(crate::strategy::empty_backtest_result());
        }
        hi = hi.min(total - 1);

        let warmup_start = lo.saturating_sub(INDICATOR_WARMUP_BARS);
        let count = hi + 1 - warmup_start;

        let buf = read_record_bytes(&path, warmup_start, count).await?;

        let offset = (lo - warmup_start) as usize;
        let window_len = (hi - lo + 1) as usize;
        let window_records = &buf[offset * KLINE_RECORD_SIZE..(offset + window_len) * KLINE_RECORD_SIZE];

        let (closes, highs, lows) = ohlc_from_records(&buf);
        let series = compute_from_ohlc(&closes, &highs, &lows);

        let window_closes: Vec<f64> = window_records
            .chunks_exact(KLINE_RECORD_SIZE)
            .map(record_close)
            .collect();
        let window_ts: Vec<i64> = unix_ts_from_records(window_records);

        let window_indicators = IndicatorSeries {
            macd_dif: series.macd_dif[offset..offset + window_len].to_vec(),
            macd_dea: series.macd_dea[offset..offset + window_len].to_vec(),
            macd_bar: series.macd_bar[offset..offset + window_len].to_vec(),
            kdj_k: series.kdj_k[offset..offset + window_len].to_vec(),
            kdj_d: series.kdj_d[offset..offset + window_len].to_vec(),
            kdj_j: series.kdj_j[offset..offset + window_len].to_vec(),
        };

        let ctx = crate::strategy::StrategyContext {
            start_index: lo,
            closes: &window_closes,
            ts_secs: &window_ts,
            indicators: &window_indicators,
        };
        catalog.run_backtest(strategy_id, ctx)
    }

    async fn resolve_path(&self, symbol: &str) -> anyhow::Result<PathBuf> {
        let mut rd = fs::read_dir(&self.dir)
            .await
            .with_context(|| format!("scan kline dir {}", self.dir.display()))?;
        while let Some(ent) = rd.next_entry().await? {
            let path = ent.path();
            if path.extension().and_then(|s| s.to_str()) != Some("bin") {
                continue;
            }
            let Some(name) = path.file_name().and_then(|n| n.to_str()) else {
                continue;
            };
            if let Some((_, sym)) = name.strip_suffix(".bin").and_then(parse_name_symbol) {
                if sym == symbol {
                    return Ok(path);
                }
            }
        }
        anyhow::bail!("no kline file for symbol {symbol}");
    }
}

fn parse_stock_from_filename(name: Option<&str>) -> Option<StockEntry> {
    let name = name?;
    let stem = name.strip_suffix(".bin")?;
    let (display_name, symbol) = parse_name_symbol(stem)?;
    Some(StockEntry {
        symbol: symbol.to_string(),
        display_name: display_name.to_string(),
    })
}

async fn read_record_bytes(path: &Path, start_index: u64, count: u64) -> anyhow::Result<Vec<u8>> {
    let mut file = fs::File::open(path).await?;
    file.seek(std::io::SeekFrom::Start(start_index * RECORD_SIZE))
        .await?;
    let mut buf = vec![0u8; (count * RECORD_SIZE) as usize];
    file.read_exact(&mut buf).await?;
    Ok(buf)
}

fn parse_name_symbol(stem: &str) -> Option<(&str, &str)> {
    let (display_name, symbol) = stem.rsplit_once('_')?;
    if symbol.is_empty() {
        return None;
    }
    Some((display_name, symbol))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_filename() {
        let s = parse_stock_from_filename(Some("立讯精密_002475.bin")).unwrap();
        assert_eq!(s.symbol, "002475");
        assert_eq!(s.display_name, "立讯精密");
    }
}
