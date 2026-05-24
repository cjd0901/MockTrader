mod api;
mod config;
mod indicators;
mod kline;
mod protocol;

use std::sync::Arc;

use tracing_subscriber::{fmt, prelude::*, EnvFilter};

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let filter = EnvFilter::try_from_default_env()
        .unwrap_or_else(|_| EnvFilter::new("info,mock_trader=info"));
    tracing_subscriber::registry()
        .with(fmt::layer())
        .with(filter)
        .init();

    let cfg = config::Config::from_env()?;
    let kline = Arc::new(kline::KlineStore::new(cfg.kline_dir.clone()));
    let state = api::AppState { kline };

    let http_listener = api::bind_http(cfg.http_listen).await?;
    let http_state = state.clone();
    let http_addr = cfg.http_listen;
    tokio::spawn(async move {
        if let Err(e) = api::run_http(http_listener, http_state).await {
            tracing::error!("http server exited: {e:#}");
        }
    });

    let tcp_listener = tokio::net::TcpListener::bind(cfg.tcp_listen).await?;
    tracing::info!(
        "tcp listening on {}, http on {}, kline dir {}",
        cfg.tcp_listen,
        http_addr,
        cfg.kline_dir.display()
    );
    api::run_tcp(tcp_listener, state).await?;
    Ok(())
}
