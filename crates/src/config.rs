use std::net::SocketAddr;
use std::path::PathBuf;

#[derive(Clone, Debug)]
pub struct Config {
    pub listen: SocketAddr,
    pub kline_dir: PathBuf,
}

impl Config {
    pub fn from_env() -> anyhow::Result<Self> {
        let host = std::env::var("TRADING_HOST").unwrap_or_else(|_| "0.0.0.0".into());
        let port: u16 = std::env::var("TRADING_PORT")
            .ok()
            .and_then(|s| s.parse().ok())
            .unwrap_or(9000);
        let listen: SocketAddr = format!("{host}:{port}")
            .parse()
            .map_err(|e| anyhow::anyhow!("invalid TRADING_HOST/TRADING_PORT: {e}"))?;

        let kline_dir = std::env::var("KLINE_DIR")
            .map(PathBuf::from)
            .unwrap_or_else(|_| PathBuf::from("data/kline/5min"));

        Ok(Self { listen, kline_dir })
    }
}
