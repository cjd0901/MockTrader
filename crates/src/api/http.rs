use std::net::SocketAddr;

use axum::{routing::get, Json, Router};
use tokio::net::TcpListener;
use tracing::info;

use super::{AppState, StockListResponse};

pub async fn bind(addr: SocketAddr) -> anyhow::Result<TcpListener> {
    Ok(TcpListener::bind(addr).await?)
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
    let stocks = state
        .kline
        .list_stocks()
        .await
        .map_err(|e| (axum::http::StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(StockListResponse { stocks }))
}
