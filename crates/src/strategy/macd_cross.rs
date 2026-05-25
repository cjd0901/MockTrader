//! MACD golden cross (buy) / death cross (sell), long-only with fixed capital.
//!
//! At most one open lot: a buy uses all cash; further golden crosses are ignored until
//! a death-cross sell closes the position.

use crate::indicators::{is_stored_valid, stored_to_f64};

use super::traits::{Strategy, StrategyContext};
use super::{SignalSide, TradeSignal};

pub struct MacdCrossStrategy;

impl Strategy for MacdCrossStrategy {
    fn id(&self) -> &'static str {
        "macd_cross"
    }

    fn generate_signals(&self, ctx: &StrategyContext<'_>) -> Vec<TradeSignal> {
        let n = ctx
            .closes
            .len()
            .min(ctx.ts_secs.len())
            .min(ctx.indicators.macd_dif.len());
        let mut out = Vec::new();
        let mut holding = false;

        for i in 1..n {
            let prev_dif = ctx.indicators.macd_dif[i - 1];
            let prev_dea = ctx.indicators.macd_dea[i - 1];
            let curr_dif = ctx.indicators.macd_dif[i];
            let curr_dea = ctx.indicators.macd_dea[i];
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
                if holding {
                    None
                } else {
                    Some(SignalSide::Buy)
                }
            } else if prev_above && curr_below {
                if !holding {
                    None
                } else {
                    Some(SignalSide::Sell)
                }
            } else {
                None
            };

            if let Some(side) = side {
                holding = matches!(side, SignalSide::Buy);
                out.push(TradeSignal {
                    bar_index: ctx.start_index + i as u64,
                    ts_sec: ctx.ts_secs[i],
                    side,
                    price: ctx.closes[i],
                });
            }
        }

        out
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::indicators::{f64_to_stored, INDICATOR_INVALID};
    use crate::strategy::traits::StrategyContext;

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

        let indicators = crate::indicators::IndicatorSeries {
            macd_dif: dif,
            macd_dea: dea,
            macd_bar: vec![INDICATOR_INVALID; 3],
            kdj_k: vec![INDICATOR_INVALID; 3],
            kdj_d: vec![INDICATOR_INVALID; 3],
            kdj_j: vec![INDICATOR_INVALID; 3],
        };

        let closes = vec![10.0, 10.1, 10.2];
        let ts = vec![1, 2, 3];
        let ctx = StrategyContext {
            start_index: 100,
            closes: &closes,
            ts_secs: &ts,
            indicators: &indicators,
        };

        let signals = MacdCrossStrategy.generate_signals(&ctx);
        assert_eq!(signals.len(), 1);
        assert_eq!(signals[0].side, SignalSide::Buy);
        assert_eq!(signals[0].bar_index, 101);
    }

    #[test]
    fn ignores_second_buy_while_holding() {
        let mut dif = vec![INDICATOR_INVALID; 5];
        let mut dea = vec![INDICATOR_INVALID; 5];
        // bar 1: golden cross (open long)
        dif[0] = f64_to_stored(0.1);
        dea[0] = f64_to_stored(0.2);
        dif[1] = f64_to_stored(0.25);
        dea[1] = f64_to_stored(0.20);
        // bars 2–3: invalid MACD (no death cross while still holding)
        // bar 4: another golden cross — must not add a second buy
        dif[3] = f64_to_stored(0.1);
        dea[3] = f64_to_stored(0.2);
        dif[4] = f64_to_stored(0.25);
        dea[4] = f64_to_stored(0.19);

        let indicators = crate::indicators::IndicatorSeries {
            macd_dif: dif,
            macd_dea: dea,
            macd_bar: vec![INDICATOR_INVALID; 5],
            kdj_k: vec![INDICATOR_INVALID; 5],
            kdj_d: vec![INDICATOR_INVALID; 5],
            kdj_j: vec![INDICATOR_INVALID; 5],
        };

        let closes = vec![10.0, 10.1, 10.2, 10.3, 10.4];
        let ts = vec![1, 2, 3, 4, 5];
        let ctx = StrategyContext {
            start_index: 0,
            closes: &closes,
            ts_secs: &ts,
            indicators: &indicators,
        };

        let signals = MacdCrossStrategy.generate_signals(&ctx);
        assert_eq!(signals.len(), 1);
        assert_eq!(signals[0].side, SignalSide::Buy);
    }

    #[test]
    fn ignores_sell_while_flat() {
        let mut dif = vec![INDICATOR_INVALID; 3];
        let mut dea = vec![INDICATOR_INVALID; 3];
        dif[0] = f64_to_stored(0.25);
        dea[0] = f64_to_stored(0.20);
        dif[1] = f64_to_stored(0.15);
        dea[1] = f64_to_stored(0.18);
        dif[2] = f64_to_stored(0.2);
        dea[2] = f64_to_stored(0.22);

        let indicators = crate::indicators::IndicatorSeries {
            macd_dif: dif,
            macd_dea: dea,
            macd_bar: vec![INDICATOR_INVALID; 3],
            kdj_k: vec![INDICATOR_INVALID; 3],
            kdj_d: vec![INDICATOR_INVALID; 3],
            kdj_j: vec![INDICATOR_INVALID; 3],
        };

        let closes = vec![10.0, 9.0, 8.0];
        let ts = vec![1, 2, 3];
        let ctx = StrategyContext {
            start_index: 0,
            closes: &closes,
            ts_secs: &ts,
            indicators: &indicators,
        };

        assert!(MacdCrossStrategy.generate_signals(&ctx).is_empty());
    }

    #[test]
    fn alternates_buy_and_sell() {
        let mut dif = vec![INDICATOR_INVALID; 5];
        let mut dea = vec![INDICATOR_INVALID; 5];
        // golden @1
        dif[0] = f64_to_stored(0.1);
        dea[0] = f64_to_stored(0.2);
        dif[1] = f64_to_stored(0.25);
        dea[1] = f64_to_stored(0.20);
        // death @2
        dif[2] = f64_to_stored(0.15);
        dea[2] = f64_to_stored(0.18);
        // golden @3
        dif[3] = f64_to_stored(0.1);
        dea[3] = f64_to_stored(0.2);
        dif[4] = f64_to_stored(0.25);
        dea[4] = f64_to_stored(0.20);

        let indicators = crate::indicators::IndicatorSeries {
            macd_dif: dif,
            macd_dea: dea,
            macd_bar: vec![INDICATOR_INVALID; 5],
            kdj_k: vec![INDICATOR_INVALID; 5],
            kdj_d: vec![INDICATOR_INVALID; 5],
            kdj_j: vec![INDICATOR_INVALID; 5],
        };

        let closes = vec![10.0, 11.0, 12.0, 13.0, 14.0];
        let ts = vec![1, 2, 3, 4, 5];
        let ctx = StrategyContext {
            start_index: 0,
            closes: &closes,
            ts_secs: &ts,
            indicators: &indicators,
        };

        let signals = MacdCrossStrategy.generate_signals(&ctx);
        assert_eq!(
            signals.iter().map(|s| s.side).collect::<Vec<_>>(),
            vec![SignalSide::Buy, SignalSide::Sell, SignalSide::Buy]
        );
    }
}
