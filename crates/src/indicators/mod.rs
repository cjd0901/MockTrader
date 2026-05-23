//! MACD (12, 26, 9) and KDJ (9) — aligned with the former Qt client logic.

const MACD_FAST: usize = 12;
const MACD_SLOW: usize = 26;
const MACD_SIGNAL: usize = 9;
const KDJ_PERIOD: usize = 9;

/// Wire scale: stored value = round(indicator × `INDICATOR_SCALE`).
pub const INDICATOR_SCALE: i32 = 100;
/// Sentinel for missing / not-yet-defined indicator values.
pub const INDICATOR_INVALID: i32 = i32::MIN;

#[derive(Debug, Clone, Default)]
pub struct IndicatorSeries {
    pub macd_dif: Vec<i32>,
    pub macd_dea: Vec<i32>,
    pub macd_bar: Vec<i32>,
    pub kdj_k: Vec<i32>,
    pub kdj_d: Vec<i32>,
    pub kdj_j: Vec<i32>,
}

pub fn f64_to_stored(v: f64) -> i32 {
    if v.is_finite() {
        (v * f64::from(INDICATOR_SCALE)).round() as i32
    } else {
        INDICATOR_INVALID
    }
}

pub fn stored_to_f64(v: i32) -> f64 {
    if v == INDICATOR_INVALID {
        f64::NAN
    } else {
        f64::from(v) / f64::from(INDICATOR_SCALE)
    }
}

pub fn is_stored_valid(v: i32) -> bool {
    v != INDICATOR_INVALID
}

pub fn compute_from_ohlc(closes: &[f64], highs: &[f64], lows: &[f64]) -> IndicatorSeries {
    let n = closes.len();
    let invalid = INDICATOR_INVALID;
    let mut out = IndicatorSeries {
        macd_dif: vec![invalid; n],
        macd_dea: vec![invalid; n],
        macd_bar: vec![invalid; n],
        kdj_k: vec![invalid; n],
        kdj_d: vec![invalid; n],
        kdj_j: vec![invalid; n],
    };

    if n >= MACD_SLOW {
        let ema_fast = ema(closes, MACD_FAST);
        let ema_slow = ema(closes, MACD_SLOW);
        for i in 0..n {
            if ema_fast[i].is_finite() && ema_slow[i].is_finite() {
                out.macd_dif[i] = f64_to_stored(ema_fast[i] - ema_slow[i]);
            }
        }
        let macd_dea_f64 = ema_skip_nan_f64(&stored_slice_to_f64(&out.macd_dif), MACD_SIGNAL);
        for i in 0..n {
            if macd_dea_f64[i].is_finite() {
                out.macd_dea[i] = f64_to_stored(macd_dea_f64[i]);
            }
        }
        for i in 0..n {
            if is_stored_valid(out.macd_dif[i]) && is_stored_valid(out.macd_dea[i]) {
                let dif = stored_to_f64(out.macd_dif[i]);
                let dea = stored_to_f64(out.macd_dea[i]);
                out.macd_bar[i] = f64_to_stored(2.0 * (dif - dea));
            }
        }
    }

    if n > 0 && KDJ_PERIOD > 0 {
        let mut k = 50.0_f64;
        let mut d = 50.0_f64;
        for i in 0..n {
            if i + 1 < KDJ_PERIOD {
                continue;
            }
            let start = i + 1 - KDJ_PERIOD;
            let mut highest = highs[i];
            let mut lowest = lows[i];
            for j in start..i {
                highest = highest.max(highs[j]);
                lowest = lowest.min(lows[j]);
            }
            let rsv = if (highest - lowest).abs() > 1e-12 {
                (closes[i] - lowest) / (highest - lowest) * 100.0
            } else {
                50.0
            };
            k = (2.0 * k + rsv) / 3.0;
            d = (2.0 * d + k) / 3.0;
            let j = 3.0 * k - 2.0 * d;
            out.kdj_k[i] = f64_to_stored(k);
            out.kdj_d[i] = f64_to_stored(d);
            out.kdj_j[i] = f64_to_stored(j);
        }
    }

    out
}

/// 6×i32 LE per bar: dif, dea, bar, k, d, j (each = round(value×100), `INDICATOR_INVALID` if N/A)
pub const INDICATOR_VALUES_SIZE: usize = 24;

fn stored_slice_to_f64(src: &[i32]) -> Vec<f64> {
    src.iter().map(|&v| stored_to_f64(v)).collect()
}

fn ema(src: &[f64], period: usize) -> Vec<f64> {
    let n = src.len();
    let mut out = vec![f64::NAN; n];
    if n < period || period == 0 {
        return out;
    }
    let sum: f64 = src[..period].iter().sum();
    out[period - 1] = sum / period as f64;
    let alpha = 2.0 / (period as f64 + 1.0);
    for i in period..n {
        out[i] = src[i] * alpha + out[i - 1] * (1.0 - alpha);
    }
    out
}

fn ema_skip_nan_f64(src: &[f64], period: usize) -> Vec<f64> {
    let n = src.len();
    let mut out = vec![f64::NAN; n];
    if period == 0 {
        return out;
    }
    let first = src.iter().position(|v| v.is_finite());
    let Some(first) = first else {
        return out;
    };
    if first + period > n {
        return out;
    }
    let sum: f64 = src[first..first + period].iter().sum();
    out[first + period - 1] = sum / period as f64;
    let alpha = 2.0 / (period as f64 + 1.0);
    for i in first + period..n {
        if !src[i].is_finite() {
            continue;
        }
        let prev = out[i - 1];
        out[i] = if prev.is_finite() {
            src[i] * alpha + prev * (1.0 - alpha)
        } else {
            src[i]
        };
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn macd_length_matches_input() {
        let closes: Vec<f64> = (0..80).map(|i| 10.0 + i as f64 * 0.01).collect();
        let highs = closes.clone();
        let lows = closes.clone();
        let s = compute_from_ohlc(&closes, &highs, &lows);
        assert_eq!(s.macd_dif.len(), 80);
        assert!(!is_stored_valid(s.macd_dif[MACD_SLOW - 2]));
        assert!(is_stored_valid(s.macd_dif[MACD_SLOW - 1]));
    }

    #[test]
    fn macd_bar_is_twice_dif_minus_dea() {
        let closes: Vec<f64> = (0..80).map(|i| 10.0 + i as f64 * 0.01).collect();
        let highs = closes.clone();
        let lows = closes.clone();
        let s = compute_from_ohlc(&closes, &highs, &lows);
        for i in 0..80 {
            if is_stored_valid(s.macd_bar[i]) {
                let expected = 2.0 * (stored_to_f64(s.macd_dif[i]) - stored_to_f64(s.macd_dea[i]));
                assert!((stored_to_f64(s.macd_bar[i]) - expected).abs() < 0.02);
            } else {
                assert!(!is_stored_valid(s.macd_bar[i]));
            }
        }
    }

    #[test]
    fn stored_roundtrip() {
        assert_eq!(f64_to_stored(1.234), 123);
        assert_eq!(f64_to_stored(f64::NAN), INDICATOR_INVALID);
        assert!((stored_to_f64(123) - 1.23).abs() < 1e-9);
        assert!(stored_to_f64(INDICATOR_INVALID).is_nan());
    }
}
