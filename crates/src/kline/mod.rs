mod parse;

use std::path::PathBuf;

use anyhow::Context;
use tokio::fs;

use crate::indicators::{self, compute_from_ohlc};
use crate::protocol::{StockEntry, KLINE_RECORD_SIZE};

pub use parse::ohlc_from_records;

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
        let path = self.resolve_path(symbol)?;
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

        let mut file = fs::File::open(&path).await?;
        use tokio::io::{AsyncReadExt, AsyncSeekExt};

        file.seek(std::io::SeekFrom::Start(warmup_start * RECORD_SIZE))
            .await?;

        let mut warmup_buf = vec![0u8; (warmup_count * RECORD_SIZE) as usize];
        file.read_exact(&mut warmup_buf).await?;

        let slice_off = ((start - warmup_start) * RECORD_SIZE) as usize;
        let records = warmup_buf[slice_off..].to_vec();

        let (closes, highs, lows) = ohlc_from_records(&warmup_buf);
        let series = compute_from_ohlc(&closes, &highs, &lows);
        let offset = (start - warmup_start) as usize;
        let window = IndicatorSeriesSlice {
            macd_dif: &series.macd_dif[offset..],
            macd_dea: &series.macd_dea[offset..],
            macd_bar: &series.macd_bar[offset..],
            kdj_k: &series.kdj_k[offset..],
            kdj_d: &series.kdj_d[offset..],
            kdj_j: &series.kdj_j[offset..],
        };
        let indicators = encode_values_slice(&window);

        debug_assert_eq!(records.len() / KLINE_RECORD_SIZE, indicators.len() / indicators::INDICATOR_VALUES_SIZE);

        Ok((start, total, records, indicators))
    }

    fn resolve_path(&self, symbol: &str) -> anyhow::Result<PathBuf> {
        for ent in std::fs::read_dir(&self.dir).with_context(|| {
            format!("scan kline dir {}", self.dir.display())
        })? {
            let ent = ent?;
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

struct IndicatorSeriesSlice<'a> {
    macd_dif: &'a [i32],
    macd_dea: &'a [i32],
    macd_bar: &'a [i32],
    kdj_k: &'a [i32],
    kdj_d: &'a [i32],
    kdj_j: &'a [i32],
}

fn encode_values_slice(s: &IndicatorSeriesSlice<'_>) -> Vec<u8> {
    let n = s.macd_dif.len();
    let mut out = Vec::with_capacity(n * indicators::INDICATOR_VALUES_SIZE);
    for i in 0..n {
        out.extend_from_slice(&s.macd_dif[i].to_le_bytes());
        out.extend_from_slice(&s.macd_dea[i].to_le_bytes());
        out.extend_from_slice(&s.macd_bar[i].to_le_bytes());
        out.extend_from_slice(&s.kdj_k[i].to_le_bytes());
        out.extend_from_slice(&s.kdj_d[i].to_le_bytes());
        out.extend_from_slice(&s.kdj_j[i].to_le_bytes());
    }
    debug_assert_eq!(out.len(), n * indicators::INDICATOR_VALUES_SIZE);
    out
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
