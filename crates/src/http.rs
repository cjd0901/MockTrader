use std::net::SocketAddr;

use axum::{routing::get, Json, Router};
use serde::Serialize;
use tokio::net::TcpListener;
use tracing::info;

use crate::state::AppState;

#[derive(Serialize)]
struct StockJson {
    symbol: String,
    #[serde(rename = "displayName")]
    display_name: String,
}

#[derive(Serialize)]
struct StockListResponse {
    stocks: Vec<StockJson>,
}

pub async fn run(listener: TcpListener, state: AppState) -> anyhow::Result<()> {
    let app = Router::new()
        .route("/api/stocks", get(list_stocks))
        .with_state(state);

    info!("http listening on {}", listener.local_addr()?);
    axum::serve(listener, app).await?;
    Ok(())
}

async fn list_stocks(
    axum::extract::State(state): axum::extract::State<AppState>,
) -> Result<Json<StockListResponse>, (axum::http::StatusCode, String)> {
    let rows = state
        .kline
        .list_stocks()
        .await
        .map_err(|e| (axum::http::StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    let stocks = rows
        .into_iter()
        .map(|s| StockJson {
            symbol: s.symbol,
            display_name: s.display_name,
        })
        .collect();

    Ok(Json(StockListResponse { stocks }))
}

pub async fn bind(addr: SocketAddr) -> anyhow::Result<TcpListener> {
    Ok(TcpListener::bind(addr).await?)
}
