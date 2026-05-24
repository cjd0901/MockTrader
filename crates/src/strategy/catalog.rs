//! Merges TOML strategy config with in-process strategy implementations.

use std::path::Path;

use tracing::warn;

use super::config::{load_strategies_file, StrategyFileEntry};
use super::registry::StrategyRegistry;
use super::traits::StrategyContext;
use super::{BacktestResult, BacktestSummary};

#[derive(Debug, Clone)]
pub struct StrategyInfo {
    pub id: String,
    pub display_name: String,
}

pub struct StrategyCatalog {
    entries: Vec<StrategyInfo>,
    registry: StrategyRegistry,
}

impl StrategyCatalog {
    pub fn from_config_path(path: &Path) -> anyhow::Result<Self> {
        let file_entries = load_strategies_file(path)?;
        Self::from_file_entries(file_entries)
    }

    fn from_file_entries(file_entries: Vec<StrategyFileEntry>) -> anyhow::Result<Self> {
        let registry = StrategyRegistry::new();
        let mut entries = Vec::new();

        for item in file_entries {
            if !item.enabled {
                continue;
            }
            if item.id.is_empty() {
                warn!("skip strategy entry with empty id");
                continue;
            }
            if !registry.contains(&item.id) {
                warn!(
                    strategy_id = %item.id,
                    "strategy in config has no server implementation, skipping"
                );
                continue;
            }
            let display_name = if item.name.is_empty() {
                item.id.clone()
            } else {
                item.name
            };
            entries.push(StrategyInfo {
                id: item.id,
                display_name,
            });
        }

        if entries.is_empty() {
            anyhow::bail!("no enabled strategies with implementations in config");
        }

        entries.sort_by(|a, b| a.id.cmp(&b.id));
        Ok(Self { entries, registry })
    }

    pub fn list(&self) -> &[StrategyInfo] {
        &self.entries
    }

    pub fn is_allowed(&self, id: &str) -> bool {
        self.entries.iter().any(|e| e.id == id)
    }

    pub fn run_backtest(
        &self,
        strategy_id: &str,
        ctx: StrategyContext<'_>,
    ) -> anyhow::Result<BacktestResult> {
        if !self.is_allowed(strategy_id) {
            anyhow::bail!("unknown or disabled strategy: {strategy_id}");
        }
        self.registry.run_backtest(strategy_id, ctx)
    }
}

pub fn empty_backtest_result() -> BacktestResult {
    BacktestResult {
        signals: Vec::new(),
        summary: BacktestSummary::empty(),
    }
}
