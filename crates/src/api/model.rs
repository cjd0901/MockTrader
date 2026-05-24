use serde::Serialize;

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
