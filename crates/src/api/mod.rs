//! Client-facing transports: HTTP stock list + TCP K-line stream.

mod http;
mod model;
mod tcp;

use std::sync::Arc;

use crate::kline::KlineStore;

pub use http::{bind as bind_http, run as run_http};
pub use model::{StockEntry, StockListResponse};
pub use tcp::run_listener as run_tcp;

#[derive(Clone)]
pub struct AppState {
    pub kline: Arc<KlineStore>,
}
