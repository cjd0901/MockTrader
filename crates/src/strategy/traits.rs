//! Core abstractions for quantitative backtest strategies.

use crate::indicators::IndicatorSeries;

use super::TradeSignal;

/// One bar window passed into a strategy (global `bar_index` = `start_index + local_index`).
#[derive(Debug, Clone, Copy)]
pub struct StrategyContext<'a> {
    pub start_index: u64,
    pub closes: &'a [f64],
    pub ts_secs: &'a [i64],
    pub indicators: &'a IndicatorSeries,
}

/// Pluggable backtest strategy: map OHLC + precomputed indicators to trade signals.
pub trait Strategy: Send + Sync {
    /// Stable API id, e.g. `macd_cross`.
    fn id(&self) -> &'static str;

    /// Human-readable name for UI / logs (reserved for strategy discovery API).
    #[allow(dead_code)]
    fn display_name(&self) -> &'static str;

    fn generate_signals(&self, ctx: &StrategyContext<'_>) -> Vec<TradeSignal>;
}
