use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize)]
pub struct StockEntry {
    pub symbol: String,
    #[serde(rename = "displayName")]
    pub display_name: String,
}

#[derive(Debug, Clone, Serialize)]
pub struct StockListResponse {
    pub stocks: Vec<StockEntry>,
}

#[derive(Debug, Clone, Serialize)]
pub struct StrategyEntryJson {
    pub id: String,
    #[serde(rename = "displayName")]
    pub display_name: String,
}

#[derive(Debug, Clone, Serialize)]
pub struct StrategyListResponse {
    pub strategies: Vec<StrategyEntryJson>,
}

#[derive(Debug, Deserialize)]
pub struct BacktestRequest {
    pub symbol: String,
    pub strategy: String,
    #[serde(rename = "startTs")]
    pub start_ts: i64,
    #[serde(rename = "endTs")]
    pub end_ts: i64,
}

#[derive(Debug, Serialize)]
pub struct BacktestSignalJson {
    #[serde(rename = "barIndex")]
    pub bar_index: u64,
    #[serde(rename = "tsSec")]
    pub ts_sec: i64,
    pub side: String,
    pub price: f64,
}

#[derive(Debug, Serialize)]
pub struct BacktestSummaryJson {
    #[serde(rename = "initialCapital")]
    pub initial_capital: f64,
    #[serde(rename = "finalEquity")]
    pub final_equity: f64,
    #[serde(rename = "totalReturnPct")]
    pub total_return_pct: f64,
    #[serde(rename = "roundTrips")]
    pub round_trips: u32,
    #[serde(rename = "winCount")]
    pub win_count: u32,
    #[serde(rename = "lossCount")]
    pub loss_count: u32,
    #[serde(rename = "openPosition")]
    pub open_position: bool,
}

#[derive(Debug, Serialize)]
pub struct BacktestResponse {
    pub signals: Vec<BacktestSignalJson>,
    pub summary: BacktestSummaryJson,
}

#[derive(Debug, Serialize)]
pub struct KlineRangeResponse {
    #[serde(rename = "minTs")]
    pub min_ts: i64,
    #[serde(rename = "maxTs")]
    pub max_ts: i64,
    #[serde(rename = "totalBars")]
    pub total_bars: u64,
}
