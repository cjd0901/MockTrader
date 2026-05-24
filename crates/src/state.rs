use std::sync::Arc;

use crate::kline::KlineStore;

#[derive(Clone)]
pub struct AppState {
    pub kline: Arc<KlineStore>,
}
