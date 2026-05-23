use crate::protocol::KLINE_RECORD_SIZE;

pub fn record_close(raw: &[u8]) -> f64 {
    let v = i32::from_le_bytes(raw[20..24].try_into().expect("close"));
    v as f64 / 100.0
}

pub fn record_high(raw: &[u8]) -> f64 {
    let v = i32::from_le_bytes(raw[12..16].try_into().expect("high"));
    v as f64 / 100.0
}

pub fn record_low(raw: &[u8]) -> f64 {
    let v = i32::from_le_bytes(raw[16..20].try_into().expect("low"));
    v as f64 / 100.0
}

pub fn ohlc_from_records(records: &[u8]) -> (Vec<f64>, Vec<f64>, Vec<f64>) {
    assert!(records.len().is_multiple_of(KLINE_RECORD_SIZE));
    let n = records.len() / KLINE_RECORD_SIZE;
    let mut closes = Vec::with_capacity(n);
    let mut highs = Vec::with_capacity(n);
    let mut lows = Vec::with_capacity(n);
    for chunk in records.chunks_exact(KLINE_RECORD_SIZE) {
        closes.push(record_close(chunk));
        highs.push(record_high(chunk));
        lows.push(record_low(chunk));
    }
    (closes, highs, lows)
}
