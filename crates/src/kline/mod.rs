//! Reads 32-byte little-endian candle records from `data/kline/5min/*.bin`.

use std::path::PathBuf;

use anyhow::Context;
use tokio::fs;

use crate::protocol::{StockEntry, KLINE_RECORD_SIZE};

const RECORD_SIZE: u64 = KLINE_RECORD_SIZE as u64;

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

    /// Returns `(start_index, total_bars, raw_record_bytes)`.
    pub async fn read_candles_raw(
        &self,
        symbol: &str,
        before_index: Option<u64>,
        limit: u32,
    ) -> anyhow::Result<(u64, u64, Vec<u8>)> {
        let path = self.resolve_path(symbol)?;
        let meta = fs::metadata(&path).await?;
        let total = meta.len() / RECORD_SIZE;
        if total == 0 {
            return Ok((0, 0, Vec::new()));
        }

        let limit = limit.min(20_000) as u64;
        let end = before_index.unwrap_or(total).min(total);
        let start = end.saturating_sub(limit);
        let count = end - start;

        let mut file = fs::File::open(&path).await?;
        use tokio::io::{AsyncReadExt, AsyncSeekExt};

        file.seek(std::io::SeekFrom::Start(start * RECORD_SIZE))
            .await?;

        let mut buf = vec![0u8; (count * RECORD_SIZE) as usize];
        file.read_exact(&mut buf).await?;

        Ok((start, total, buf))
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
