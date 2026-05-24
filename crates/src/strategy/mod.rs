//! Quantitative strategies for backtesting.

mod pnl;

use crate::indicators::{is_stored_valid, stored_to_f64, IndicatorSeries};

pub use pnl::{compute_pnl, BacktestSummary};

#[derive(Debug, Clone)]
pub struct BacktestResult {
    pub signals: Vec<TradeSignal>,
    pub summary: BacktestSummary,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SignalSide {
    Buy,
    Sell,
}

#[derive(Debug, Clone)]
pub struct TradeSignal {
    pub bar_index: u64,
    pub ts_sec: i64,
    pub side: SignalSide,
    pub price: f64,
}

#[derive(Debug, Clone, Copy)]
pub enum StrategyKind {
    MacdCross,
}

impl StrategyKind {
    pub fn parse(s: &str) -> Option<Self> {
        match s {
            "macd_cross" => Some(Self::MacdCross),
            _ => None,
        }
    }
}

pub fn run(
    kind: StrategyKind,
    start_index: u64,
    closes: &[f64],
    ts_secs: &[i64],
    indicators: &IndicatorSeries,
) -> Vec<TradeSignal> {
    match kind {
        StrategyKind::MacdCross => run_macd_cross(start_index, closes, ts_secs, indicators),
    }
}

/// MACD 金叉（DIF 上穿 DEA）买入，死叉卖出。
fn run_macd_cross(
    start_index: u64,
    closes: &[f64],
    ts_secs: &[i64],
    indicators: &IndicatorSeries,
) -> Vec<TradeSignal> {
    let n = closes.len().min(ts_secs.len()).min(indicators.macd_dif.len());
    let mut out = Vec::new();

    for i in 1..n {
        let prev_dif = indicators.macd_dif[i - 1];
        let prev_dea = indicators.macd_dea[i - 1];
        let curr_dif = indicators.macd_dif[i];
        let curr_dea = indicators.macd_dea[i];
        if !is_stored_valid(prev_dif)
            || !is_stored_valid(prev_dea)
            || !is_stored_valid(curr_dif)
            || !is_stored_valid(curr_dea)
        {
            continue;
        }

        let prev_dif = stored_to_f64(prev_dif);
        let prev_dea = stored_to_f64(prev_dea);
        let curr_dif = stored_to_f64(curr_dif);
        let curr_dea = stored_to_f64(curr_dea);

        let prev_below = prev_dif <= prev_dea;
        let curr_above = curr_dif > curr_dea;
        let prev_above = prev_dif >= prev_dea;
        let curr_below = curr_dif < curr_dea;

        let side = if prev_below && curr_above {
            Some(SignalSide::Buy)
        } else if prev_above && curr_below {
            Some(SignalSide::Sell)
        } else {
            None
        };

        if let Some(side) = side {
            out.push(TradeSignal {
                bar_index: start_index + i as u64,
                ts_sec: ts_secs[i],
                side,
                price: closes[i],
            });
        }
    }

    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::indicators::{f64_to_stored, INDICATOR_INVALID};

    #[test]
    fn detects_golden_cross() {
        let mut dif = vec![INDICATOR_INVALID; 3];
        let mut dea = vec![INDICATOR_INVALID; 3];
        dif[0] = f64_to_stored(0.1);
        dea[0] = f64_to_stored(0.2);
        dif[1] = f64_to_stored(0.25);
        dea[1] = f64_to_stored(0.20);
        dif[2] = f64_to_stored(0.2);
        dea[2] = f64_to_stored(0.18);

        let indicators = IndicatorSeries {
            macd_dif: dif,
            macd_dea: dea,
            macd_bar: vec![INDICATOR_INVALID; 3],
            kdj_k: vec![INDICATOR_INVALID; 3],
            kdj_d: vec![INDICATOR_INVALID; 3],
            kdj_j: vec![INDICATOR_INVALID; 3],
        };

        let closes = vec![10.0, 10.1, 10.2];
        let ts = vec![1, 2, 3];
        let signals = run_macd_cross(100, &closes, &ts, &indicators);
        assert_eq!(signals.len(), 1);
        assert_eq!(signals[0].side, SignalSide::Buy);
        assert_eq!(signals[0].bar_index, 101);
    }
}
