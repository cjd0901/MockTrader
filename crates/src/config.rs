use std::net::SocketAddr;
use std::path::PathBuf;

#[derive(Clone, Debug)]
pub struct Config {
    pub tcp_listen: SocketAddr,
    pub http_listen: SocketAddr,
    pub kline_dir: PathBuf,
    pub strategies_file: PathBuf,
}

impl Config {
    pub fn from_env() -> anyhow::Result<Self> {
        let tcp_host = std::env::var("TRADING_HOST").unwrap_or_else(|_| "0.0.0.0".into());
        let tcp_port: u16 = std::env::var("TRADING_PORT")
            .ok()
            .and_then(|s| s.parse().ok())
            .unwrap_or(9000);
        let tcp_listen: SocketAddr = format!("{tcp_host}:{tcp_port}")
            .parse()
            .map_err(|e| anyhow::anyhow!("invalid TRADING_HOST/TRADING_PORT: {e}"))?;

        let http_host =
            std::env::var("TRADING_HTTP_HOST").unwrap_or_else(|_| "0.0.0.0".into());
        let http_port: u16 = std::env::var("TRADING_HTTP_PORT")
            .ok()
            .and_then(|s| s.parse().ok())
            .unwrap_or(9080);
        let http_listen: SocketAddr = format!("{http_host}:{http_port}")
            .parse()
            .map_err(|e| anyhow::anyhow!("invalid TRADING_HTTP_HOST/TRADING_HTTP_PORT: {e}"))?;

        let kline_dir = std::env::var("KLINE_DIR")
            .map(PathBuf::from)
            .unwrap_or_else(|_| PathBuf::from("data/kline/5min"));

        let strategies_file = std::env::var("STRATEGIES_FILE")
            .map(PathBuf::from)
            .unwrap_or_else(|_| PathBuf::from("data/config/strategies.toml"));

        Ok(Self {
            tcp_listen,
            http_listen,
            kline_dir,
            strategies_file,
        })
    }
}
