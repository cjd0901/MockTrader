# MockTrader Architecture

## Overview

MockTrader is split into a **Rust server** and a **Qt 6 desktop client**. Market data lives in local `.bin` files; the server reads them and exposes two transports:

| Transport | Port (default) | Purpose |
|-----------|----------------|---------|
| HTTP (Axum) | 9080 | Stock list (`GET /api/stocks`) |
| TCP (binary frames) | 9000 | K-line + indicator chunks (`GetCandles`) |

## Server (`crates/`)

```
src/
  main.rs          # boots HTTP + TCP
  config.rs        # env: TRADING_*_HOST/PORT, KLINE_DIR
  api/             # client-facing transports
    mod.rs         # AppState, re-exports
    model.rs       # StockEntry, StockListResponse (JSON)
    http.rs        # Axum GET /api/stocks
    tcp.rs         # per-connection TCP handler
  kline/           # .bin I/O, indicator warmup, encode window
  indicators/      # MACD/KDJ → i32×100 wire values
  protocol/
    candle.rs      # TCP framing & GetCandles codec only
```

Data flow for candles:

1. Client sends `GetCandles` over TCP.
2. `KlineStore::read_candles_raw` loads warmup bars, computes indicators, returns records + indicator bytes for the requested window.
3. Server replies with `CandleChunk` (start index, total, raw bytes).

## Client (`client/src/`)

```
app/           # theme, branding, load limits (KlineLoadConfig)
model/         # CandleBar, StockRow, IndicatorBar
protocol/      # TcpCandleCodec (encode/decode TCP + bin records)
api/           # HttpStockClient, TcpCandleClient
pages/         # HomePage, StockDetailPage
widgets/       # timeline, list delegate
MainWindow.cpp # wires HTTP list + TCP candles
```

- **Home**: `HttpStockClient` → `GET /api/stocks` → `HomePage::setStocks`.
- **Detail**: `TcpCandleClient` → binary `GetCandles` → charts.

## On-disk K-line format

32 bytes per bar (little-endian `i32` fields). See README for field layout.

## Indicator wire format

24 bytes per bar: 6× `i32` LE (`macd_dif`, `macd_dea`, `macd_bar`, `kdj_k`, `kdj_d`, `kdj_j`), value = `round(indicator × 100)`, `i32::MIN` = invalid.
