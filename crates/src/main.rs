mod config;
mod indicators;
mod kline;
mod protocol;
mod tcp;

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
    let state = tcp::AppState { kline };

    let listener = tokio::net::TcpListener::bind(cfg.listen).await?;
    tracing::info!(
        "tcp listening on {}, kline dir {}",
        cfg.listen,
        cfg.kline_dir.display()
    );
    tcp::run_listener(listener, state).await?;
    Ok(())
}
