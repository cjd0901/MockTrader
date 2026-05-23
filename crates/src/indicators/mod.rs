const MACD_FAST: usize = 12;
const MACD_SLOW: usize = 26;
const MACD_SIGNAL: usize = 9;
const KDJ_PERIOD: usize = 9;

#[derive(Debug, Clone, Default)]
pub struct IndicatorSeries {
    pub macd_dif: Vec<f64>,
    pub macd_dea: Vec<f64>,
    /// MACD 柱 = 2×(DIF−DEA)
    pub macd_bar: Vec<f64>,
    pub kdj_k: Vec<f64>,
    pub kdj_d: Vec<f64>,
    pub kdj_j: Vec<f64>,
}

pub fn compute_from_ohlc(closes: &[f64], highs: &[f64], lows: &[f64]) -> IndicatorSeries {
    let n = closes.len();
    let mut out = IndicatorSeries {
        macd_dif: vec![f64::NAN; n],
        macd_dea: vec![f64::NAN; n],
        macd_bar: vec![f64::NAN; n],
        kdj_k: vec![f64::NAN; n],
        kdj_d: vec![f64::NAN; n],
        kdj_j: vec![f64::NAN; n],
    };

    if n >= MACD_SLOW {
        let ema_fast = ema(closes, MACD_FAST);
        let ema_slow = ema(closes, MACD_SLOW);
        for i in 0..n {
            if ema_fast[i].is_finite() && ema_slow[i].is_finite() {
                out.macd_dif[i] = ema_fast[i] - ema_slow[i];
            }
        }
        out.macd_dea = ema_skip_nan(&out.macd_dif, MACD_SIGNAL);
        for i in 0..n {
            if out.macd_dif[i].is_finite() && out.macd_dea[i].is_finite() {
                out.macd_bar[i] = 2.0 * (out.macd_dif[i] - out.macd_dea[i]);
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
            out.kdj_k[i] = k;
            out.kdj_d[i] = d;
            out.kdj_j[i] = j;
        }
    }

    out
}

/// 6×f64 LE per bar: dif, dea, bar, k, d, j
pub const INDICATOR_VALUES_SIZE: usize = 48;

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

fn ema_skip_nan(src: &[f64], period: usize) -> Vec<f64> {
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
        assert!(s.macd_dif[MACD_SLOW - 2].is_nan());
        assert!(s.macd_dif[MACD_SLOW - 1].is_finite());
    }

    #[test]
    fn macd_bar_is_twice_dif_minus_dea() {
        let closes: Vec<f64> = (0..80).map(|i| 10.0 + i as f64 * 0.01).collect();
        let highs = closes.clone();
        let lows = closes.clone();
        let s = compute_from_ohlc(&closes, &highs, &lows);
        for i in 0..80 {
            if s.macd_bar[i].is_finite() {
                let expected = 2.0 * (s.macd_dif[i] - s.macd_dea[i]);
                assert!((s.macd_bar[i] - expected).abs() < 1e-9);
            } else {
                assert!(s.macd_bar[i].is_nan());
            }
        }
    }
}
