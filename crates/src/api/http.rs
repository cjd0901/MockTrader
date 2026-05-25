use serde::Deserialize;
use axum::{
    extract::Query,
    routing::{get, post},
    Json, Router,
};
use tokio::net::TcpListener;
use tracing::info;

use crate::strategy::SignalSide;

use super::model::{
    BacktestRequest, BacktestResponse, BacktestSignalJson, BacktestSummaryJson, KlineRangeResponse,
    StockListResponse, StrategyEntryJson, StrategyListResponse,
};
use super::AppState;

pub async fn run(listener: TcpListener, state: AppState) -> anyhow::Result<()> {
    let app = Router::new()
        .route("/api/stocks", get(list_stocks))
        .route("/api/strategies", get(list_strategies))
        .route("/api/kline/range", get(kline_range))
        .route("/api/backtest", post(run_backtest))
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
        .map_err(internal)?;

    Ok(Json(StockListResponse { stocks }))
}

#[derive(Debug, Deserialize)]
struct KlineRangeQuery {
    symbol: String,
}

async fn list_strategies(
    axum::extract::State(state): axum::extract::State<AppState>,
) -> Json<StrategyListResponse> {
    let strategies = state
        .strategies
        .list()
        .iter()
        .map(|s| StrategyEntryJson {
            id: s.id.clone(),
            display_name: s.display_name.clone(),
        })
        .collect();
    Json(StrategyListResponse { strategies })
}

async fn kline_range(
    axum::extract::State(state): axum::extract::State<AppState>,
    Query(q): Query<KlineRangeQuery>,
) -> Result<Json<KlineRangeResponse>, (axum::http::StatusCode, String)> {
    let (min_ts, max_ts, total_bars) = state
        .kline
        .file_time_range(&q.symbol)
        .await
        .map_err(internal)?;

    Ok(Json(KlineRangeResponse {
        min_ts,
        max_ts,
        total_bars,
    }))
}

async fn run_backtest(
    axum::extract::State(state): axum::extract::State<AppState>,
    Json(req): Json<BacktestRequest>,
) -> Result<Json<BacktestResponse>, (axum::http::StatusCode, String)> {
    if !state.strategies.is_allowed(&req.strategy) {
        return Err((
            axum::http::StatusCode::BAD_REQUEST,
            format!("unknown or disabled strategy: {}", req.strategy),
        ));
    }

    let result = state
        .kline
        .run_backtest(state.strategies.as_ref(), &req.symbol, &req.strategy, req.start_ts, req.end_ts)
        .await
        .map_err(internal)?;

    let json_signals = result
        .signals
        .into_iter()
        .map(|s| BacktestSignalJson {
            bar_index: s.bar_index,
            ts_sec: s.ts_sec,
            side: match s.side {
                SignalSide::Buy => "buy".to_string(),
                SignalSide::Sell => "sell".to_string(),
            },
            price: s.price,
        })
        .collect();

    let s = result.summary;
    let summary = BacktestSummaryJson {
        initial_capital: s.initial_capital,
        final_equity: s.final_equity,
        total_return_pct: s.total_return_pct,
        round_trips: s.round_trips,
        win_count: s.win_count,
        loss_count: s.loss_count,
        open_position: s.open_position,
    };

    Ok(Json(BacktestResponse {
        signals: json_signals,
        summary,
    }))
}

fn internal(e: impl std::fmt::Display) -> (axum::http::StatusCode, String) {
    (axum::http::StatusCode::INTERNAL_SERVER_ERROR, e.to_string())
}
