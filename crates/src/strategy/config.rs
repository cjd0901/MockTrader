//! Strategy list loaded from TOML (`data/config/strategies.toml` by default).

use std::path::Path;

use anyhow::Context;
use serde::Deserialize;

#[derive(Debug, Clone, Deserialize)]
pub struct StrategyFileEntry {
    pub id: String,
    pub name: String,
    #[serde(default = "default_enabled")]
    pub enabled: bool,
}

fn default_enabled() -> bool {
    true
}

#[derive(Debug, Deserialize)]
struct StrategiesFile {
    #[serde(rename = "strategy")]
    strategies: Vec<StrategyFileEntry>,
}

pub fn load_strategies_file(path: &Path) -> anyhow::Result<Vec<StrategyFileEntry>> {
    let text = std::fs::read_to_string(path)
        .with_context(|| format!("read strategies file {}", path.display()))?;
    let file: StrategiesFile = toml::from_str(&text)
        .with_context(|| format!("parse strategies TOML {}", path.display()))?;
    Ok(file.strategies)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_strategy_table() {
        let text = r#"
[[strategy]]
id = "macd_cross"
name = "MACD cross"
enabled = true

[[strategy]]
id = "disabled_algo"
name = "Disabled"
enabled = false
"#;
        let file: StrategiesFile = toml::from_str(text).unwrap();
        assert_eq!(file.strategies.len(), 2);
        assert!(file.strategies[0].enabled);
        assert!(!file.strategies[1].enabled);
    }
}
