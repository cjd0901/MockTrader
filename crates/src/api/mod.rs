//! Client-facing transports: HTTP stock list + TCP K-line stream.

mod http;
mod model;
mod tcp;

use std::sync::Arc;

use crate::kline::KlineStore;
use crate::strategy::StrategyCatalog;

pub use http::run as run_http;
pub use model::StockEntry;
pub use tcp::run_listener as run_tcp;

#[derive(Clone)]
pub struct AppState {
    pub kline: Arc<KlineStore>,
    pub strategies: Arc<StrategyCatalog>,
}
