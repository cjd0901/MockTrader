# MockTrader Architecture

## Overview

MockTrader is split into a **Rust server** and a **Qt 6 desktop client**. Market data lives in local `.bin` files; the server reads them and exposes two transports:

| Transport | Port (default) | Purpose |
|-----------|----------------|---------|
| HTTP (Axum) | 9080 | Stock list, strategy list, K-line file time range, backtest |
| TCP (binary frames) | 9000 | K-line + indicator chunks (`GetCandles`) |

## Server (`crates/`)

```
src/
  main.rs          # boots HTTP + TCP
  config.rs        # env: TRADING_*_HOST/PORT, KLINE_DIR
  api/             # client-facing transports
    mod.rs         # AppState
    model.rs       # JSON request/response types
    http.rs        # GET /api/stocks, /api/strategies, /api/kline/range; POST /api/backtest
    tcp.rs         # per-connection TCP handler
  kline/           # .bin I/O, binary search, backtest window
    parse.rs       # OHLC + unix timestamp from records
    range.rs       # file min/max ts, index by timestamp
  strategy/        # Strategy trait, TOML catalog, registry, MACD cross, PnL
  indicators/      # MACD/KDJ → i32×100 wire values
  protocol/
    candle.rs      # TCP framing & GetCandles codec
```

**Candle path:** client `GetCandles` → `KlineStore::read_candles_raw` (warmup + indicators) → `CandleChunk`.

**Backtest path:** `POST /api/backtest` → binary search `[startTs, endTs]` → strategy signals + `compute_pnl`.

## Client (`client/src/`)

```
app/           # theme, branding, ServerConfig (env host/ports), KlineLoadConfig
api/           # HttpApiUrl, HttpStockClient, HttpBacktestClient, TcpCandleClient
model/         # CandleBar, StockRow, IndicatorBar, backtest types
protocol/      # TcpCandleCodec
pages/         # HomePage, StockDetailPage (chart + BacktestPanel)
widgets/       # KlineTimelineBar, StrategyPicker, StockListDelegate, BacktestPanel
MainWindow.cpp # wires HTTP list/range/backtest + TCP candles
```

- **Home:** `GET /api/stocks`.
- **Startup:** `GET /api/strategies` → backtest strategy picker.
- **Detail:** TCP candles; `GET /api/kline/range` for backtest time bounds; `POST /api/backtest` for signals/PnL overlay.

Strategy metadata lives in `data/strategies.toml` (`STRATEGIES_FILE`); only `enabled` entries with a matching `Strategy` implementation are exposed and runnable.

## On-disk K-line format

32 bytes per bar (little-endian `i32` fields). See README for field layout.

## Indicator wire format

24 bytes per bar: 6× `i32` LE (`macd_dif`, `macd_dea`, `macd_bar`, `kdj_k`, `kdj_d`, `kdj_j`), value = `round(indicator × 100)`, `i32::MIN` = invalid.
