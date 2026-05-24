//! In-process strategy implementations (lookup by API id).

use std::collections::HashMap;

use super::macd_cross::MacdCrossStrategy;
use super::traits::{Strategy, StrategyContext};
use super::{compute_pnl, BacktestResult};

pub struct StrategyRegistry {
    strategies: HashMap<&'static str, Box<dyn Strategy>>,
}

impl StrategyRegistry {
    pub fn new() -> Self {
        let mut strategies: HashMap<&'static str, Box<dyn Strategy>> = HashMap::new();
        Self::register(&mut strategies, MacdCrossStrategy);
        Self { strategies }
    }

    fn register(map: &mut HashMap<&'static str, Box<dyn Strategy>>, strategy: impl Strategy + 'static) {
        let boxed: Box<dyn Strategy> = Box::new(strategy);
        map.insert(boxed.id(), boxed);
    }

    pub fn contains(&self, id: &str) -> bool {
        self.strategies.contains_key(id)
    }

    pub fn run_backtest(&self, strategy_id: &str, ctx: StrategyContext<'_>) -> anyhow::Result<BacktestResult> {
        let strategy = self
            .strategies
            .get(strategy_id)
            .map(|b| b.as_ref())
            .ok_or_else(|| anyhow::anyhow!("no implementation for strategy: {strategy_id}"))?;
        let signals = strategy.generate_signals(&ctx);
        let end_price = ctx.closes.last().copied();
        let summary = compute_pnl(&signals, end_price);
        Ok(BacktestResult { signals, summary })
    }
}
