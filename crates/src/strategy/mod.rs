//! Quantitative strategies for backtesting.

mod catalog;
mod config;
mod macd_cross;
mod pnl;
mod registry;
mod traits;

pub use catalog::{empty_backtest_result, StrategyCatalog};
pub use pnl::{compute_pnl, BacktestSummary};
pub use traits::StrategyContext;

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
