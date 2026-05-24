use std::path::Path;

use tokio::io::{AsyncReadExt, AsyncSeekExt};

use super::parse::record_unix_ts;
use crate::protocol::KLINE_RECORD_SIZE;

const RECORD_SIZE: u64 = KLINE_RECORD_SIZE as u64;

pub async fn read_record_at(path: &Path, index: u64) -> anyhow::Result<[u8; KLINE_RECORD_SIZE]> {
    let mut file = tokio::fs::File::open(path).await?;
    file.seek(std::io::SeekFrom::Start(index * RECORD_SIZE))
        .await?;
    let mut buf = [0u8; KLINE_RECORD_SIZE];
    file.read_exact(&mut buf).await?;
    Ok(buf)
}

/// First bar index with `unix_ts >= target` (assumes file sorted by time).
pub async fn find_first_index_ge(path: &Path, total: u64, target_ts: i64) -> anyhow::Result<u64> {
    if total == 0 {
        return Ok(0);
    }
    let last = read_record_at(path, total - 1).await?;
    if record_unix_ts(&last) < target_ts {
        return Ok(total);
    }
    let first = read_record_at(path, 0).await?;
    if record_unix_ts(&first) >= target_ts {
        return Ok(0);
    }

    let mut lo = 0i64;
    let mut hi = total as i64 - 1;
    while lo < hi {
        let mid = lo + (hi - lo) / 2;
        let rec = read_record_at(path, mid as u64).await?;
        if record_unix_ts(&rec) < target_ts {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    Ok(lo as u64)
}

/// Last bar index with `unix_ts <= target`.
/// First and last bar unix timestamps in the file (read from records 0 and `total - 1`).
pub async fn file_unix_range(path: &Path, total: u64) -> anyhow::Result<(i64, i64)> {
    if total == 0 {
        anyhow::bail!("empty kline file");
    }
    let first = read_record_at(path, 0).await?;
    let last = read_record_at(path, total - 1).await?;
    Ok((record_unix_ts(&first), record_unix_ts(&last)))
}

pub async fn find_last_index_le(path: &Path, total: u64, target_ts: i64) -> anyhow::Result<u64> {
    if total == 0 {
        return Ok(0);
    }
    let first = read_record_at(path, 0).await?;
    if record_unix_ts(&first) > target_ts {
        return Ok(0);
    }
    let last = read_record_at(path, total - 1).await?;
    if record_unix_ts(&last) <= target_ts {
        return Ok(total - 1);
    }

    let mut lo = 0i64;
    let mut hi = total as i64 - 1;
    while lo < hi {
        let mid = lo + (hi - lo + 1) / 2;
        let rec = read_record_at(path, mid as u64).await?;
        if record_unix_ts(&rec) > target_ts {
            hi = mid - 1;
        } else {
            lo = mid;
        }
    }
    Ok(lo as u64)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn binary_search_on_synthetic_file() -> anyhow::Result<()> {
        let dir = std::env::temp_dir().join(format!("mocktrader_range_{}", std::process::id()));
        std::fs::create_dir_all(&dir)?;
        let path = dir.join("test.bin");

        let mut bytes = Vec::new();
        for i in 0..10i32 {
            let date = 20240102i32;
            // +300 keeps minutes < 60 (93000, 93300, … 95700).
            let time = 93000 + i * 300;
            let price = 1000 + i * 10;
            bytes.extend_from_slice(&date.to_le_bytes());
            bytes.extend_from_slice(&time.to_le_bytes());
            for _ in 0..6 {
                bytes.extend_from_slice(&price.to_le_bytes());
            }
        }
        tokio::fs::write(&path, &bytes).await?;

        let total = bytes.len() as u64 / RECORD_SIZE;
        let t5 = record_unix_ts(&read_record_at(&path, 5).await?);
        let idx = find_first_index_ge(&path, total, t5).await?;
        assert_eq!(idx, 5);
        let last_le = find_last_index_le(&path, total, t5).await?;
        assert_eq!(last_le, 5);

        let (file_min, file_max) = file_unix_range(&path, total).await?;
        assert_eq!(file_min, record_unix_ts(&read_record_at(&path, 0).await?));
        assert_eq!(file_max, record_unix_ts(&read_record_at(&path, total - 1).await?));

        let _ = std::fs::remove_dir_all(dir);
        Ok(())
    }
}
