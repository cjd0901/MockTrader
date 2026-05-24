use chrono::{NaiveDate, NaiveDateTime, NaiveTime, TimeZone};
use chrono_tz::Tz;

use crate::protocol::kline_record::{self, SIZE as KLINE_RECORD_SIZE};

const TZ: Tz = chrono_tz::Asia::Shanghai;

pub fn record_close(raw: &[u8]) -> f64 {
    kline_record::price_from_i32(kline_record::read_i32(raw, kline_record::off::CLOSE))
}

pub fn record_high(raw: &[u8]) -> f64 {
    kline_record::price_from_i32(kline_record::read_i32(raw, kline_record::off::HIGH))
}

pub fn record_low(raw: &[u8]) -> f64 {
    kline_record::price_from_i32(kline_record::read_i32(raw, kline_record::off::LOW))
}

pub fn record_unix_ts(raw: &[u8]) -> i64 {
    let date = kline_record::read_i32(raw, kline_record::off::DATE);
    let time = kline_record::read_i32(raw, kline_record::off::TIME);
    let y = date / 10000;
    let m = (date / 100) % 100;
    let d = date % 100;
    let hh = time / 10000;
    let mm = (time / 100) % 100;
    let ss = time % 100;

    let date = NaiveDate::from_ymd_opt(y, m as u32, d as u32).unwrap_or(NaiveDate::MIN);
    let time = NaiveTime::from_hms_opt(hh as u32, mm as u32, ss as u32).unwrap_or(NaiveTime::MIN);
    let dt = NaiveDateTime::new(date, time);
    TZ.from_local_datetime(&dt)
        .single()
        .map(|t| t.timestamp())
        .unwrap_or(0)
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

pub fn unix_ts_from_records(records: &[u8]) -> Vec<i64> {
    assert!(records.len().is_multiple_of(KLINE_RECORD_SIZE));
    records
        .chunks_exact(KLINE_RECORD_SIZE)
        .map(record_unix_ts)
        .collect()
}
