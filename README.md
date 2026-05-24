<div align="center">

<img src="docs/logo.png" alt="MockTrader" width="160" />

# MockTrader

**Quantitative historical backtesting with simulated execution and chart review**

[中文文档](README.zh-CN.md)

</div>

---

MockTrader is a **local-first quantitative backtesting platform** for Chinese A-share (and similar) intraday history. It replays historical 5-minute bars, runs **custom indicator strategies**, **simulates order placement**, and **splits large parent orders into child fills**—then overlays **buy/sell markers** on the candlestick chart and reports **PnL / returns**.

The stack is a **Rust TCP server** (data + indicators + future simulation engine) and a **Qt 6 desktop client** (charts, controls, results).

## Screenshot

![MockTrader desktop client](docs/example.png)

## Highlights

| Capability | Description |
|------------|-------------|
| **Strategy backtest** | Run your own quantitative rules on historical bars and step through time bar-by-bar. |
| **Simulated orders** | Model automatic buy/sell signals with realistic timing on each bar. |
| **Large-order splitting** | Simulate breaking a parent block order into smaller child orders over multiple bars. |
| **Chart review** | Candlesticks with MACD / KDJ; inspect entries, exits, and trade markers on the K-line. |
| **Performance** | Review per-trade and aggregate returns, win rate, and equity curve (planned UI). |
| **Local data** | No cloud dependency; K-lines stored as compact binary files on disk. |

## Status

| Area | State |
|------|--------|
| 5-minute K-line storage & TCP API | **Available** |
| Candlestick chart, timeline scroll, prefetch | **Available** |
| MACD / KDJ (server-side, 24-byte indicator payload, i32×100) | **Available** |
| Strategy engine, order simulator, TWAP/split logic | **Planned** |
| Buy/sell markers & PnL panel on chart | **Planned** |

> The README describes the **product direction**. Implemented pieces are marked **Available**; trading simulation and backtest reporting are on the roadmap.

## Architecture

```
MockTrader/
├── Cargo.toml
├── crates/src/api/         # server HTTP + TCP + API models
├── client/src/
│   ├── app/                # theme, branding, load config
│   ├── model/              # shared market types
│   ├── protocol/           # TCP candle codec
│   ├── api/                # HTTP stock list + TCP candles
│   ├── pages/              # home & detail UI
│   └── widgets/
├── docs/
│   ├── logo.png
│   ├── example.png
│   └── ARCHITECTURE.md
└── data/
    ├── scripts/get_5min.py
    └── kline/5min/
```

```mermaid
flowchart LR
  subgraph client [Qt Client]
    UI[Charts & controls]
    BT[Backtest UI - planned]
  end
  subgraph server [Rust Server]
    IO[K-line reader]
    IND[Indicators MACD/KDJ]
    SIM[Order simulator - planned]
  end
  BIN[(.bin files)]
  UI -->|HTTP JSON| IO
  UI -->|TCP binary| IO
  IO --> BIN
  IND --> IO
  SIM --> IO
  BT --> UI
```

## Prepare K-line data

Requires Python 3, `baostock`, and `pandas`.

```bash
pip install baostock pandas
python data/scripts/get_5min.py
```

Output example: `data/kline/5min/立讯精密_002475.bin`.

### Binary record (32 bytes, little-endian `i32` × 8)

| Field | Description |
|-------|-------------|
| `date` | `YYYYMMDD` |
| `time` | `HHmmss` (e.g. `093500` from baostock time string) |
| `open` / `high` / `low` / `close` | Price × 100 as integer |
| `volume` | Volume |
| `amount` | Amount ÷ 10000 as integer |

File name: `{display_name}_{symbol}.bin`.

## Run the server

From the repository root:

```bash
cargo run -p mock-trader
```

| Variable | Default | Purpose |
|----------|---------|---------|
| `TRADING_HOST` | `0.0.0.0` | TCP bind (K-line / indicators) |
| `TRADING_PORT` | `9000` | TCP port |
| `TRADING_HTTP_HOST` | `0.0.0.0` | HTTP bind (stock list) |
| `TRADING_HTTP_PORT` | `9080` | HTTP port |
| `KLINE_DIR` | `data/kline/5min` | K-line `.bin` directory |

## Build & run the client

Requires **Qt 6.5+** (Core, Gui, Widgets, Network, Charts).

```bash
cd client
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build -j
./build/mock_trader_client
```

| Variable | Default | Purpose |
|----------|---------|---------|
| `TRADING_HTTP_HOST` | `127.0.0.1` | HTTP stock list |
| `TRADING_HTTP_PORT` | `9080` | HTTP port |
| `TRADING_TCP_HOST` | `127.0.0.1` | TCP K-line stream |
| `TRADING_TCP_PORT` | `9000` | TCP port |

## Client workflow (current)

1. Start the server, then launch the client. The home screen loads the stock list via **HTTP** (`GET /api/stocks`).
2. Open a symbol to load recent 5-minute history over **TCP** (~180 trading days in memory) and show the **latest 100 bars** by default.
3. Drag the **timeline** at the bottom to scroll; older bars are prefetched near the left edge.
4. View **MACD** (DIF, DEA, histogram) and **KDJ** computed on the server; colored readouts match chart lines.
5. Double-click the main chart for OHLC detail; high/low labels track the visible window.
6. Use the **backtest panel** on the right: pick a strategy (e.g. MACD golden cross / death cross), set a time range within the loaded bars, then run. The server binary-searches the `.bin` file for that range and returns buy/sell markers on the chart.

Tune bar counts in `client/src/KlineLoadConfig.h` (`VisibleBarCount`, `InitialBarLimit`, etc.).

## HTTP API

### Stock list

`GET /api/stocks` → JSON:

```json
{
  "stocks": [
    { "symbol": "002475", "displayName": "立讯精密" }
  ]
}
```

### K-line file time range

`GET /api/kline/range?symbol=002475` → JSON:

```json
{ "minTs": 1704159000, "maxTs": 1710000000, "totalBars": 12000 }
```

Timestamps are read from the first and last records in the symbol `.bin` file (used for backtest time pickers and binary search bounds).

### Backtest

`POST /api/backtest` with JSON body:

```json
{
  "symbol": "002475",
  "strategy": "macd_cross",
  "startTs": 1704159000,
  "endTs": 1704245400
}
```

Response:

```json
{
  "signals": [
    { "barIndex": 1200, "tsSec": 1704180000, "side": "buy", "price": 10.52 }
  ],
  "summary": {
    "initialCapital": 100000,
    "finalEquity": 105230.5,
    "totalReturnPct": 5.23,
    "roundTrips": 12,
    "winCount": 7,
    "lossCount": 5,
    "openPosition": false
  }
}
```

`summary` simulates long-only full-capital trades (100k initial); open lots at period end are marked to the last close. Strategies: `macd_cross` (DIF crosses above DEA → buy, below → sell).

## TCP protocol (K-line / indicators)

Frame: `[u8 type][u32 LE payload length][payload]`

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| `GetCandles` | 2 | C→S | `symbol` + optional `before_index` + `limit` |
| `CandleChunk` | 102 | S→C | `start_index`, `total`, raw K-line bytes, indicator bytes |
| `Error` | 255 | S→C | Error text |

Each bar: **32-byte candle** + **24-byte indicators** (6× `i32` LE, value = round(indicator×100); `i32::MIN` = invalid: `macd_dif`, `macd_dea`, `macd_bar`, `kdj_k`, `kdj_d`, `kdj_j`).

`GetCandles` without `before_index` returns the latest `limit` records at the end of the file.

Details: `docs/ARCHITECTURE.md`, `crates/src/protocol/candle.rs`.

## Development

```bash
cargo test -p mock-trader
cargo clean
rm -rf client/build
```

CI workflows: `.github/workflows/mock_trader_server.yml`, `mock_trader_client.yml`.

`.gitignore` excludes `target/`, `client/build/`, local `*.bin`, and IDE metadata.

## License

See repository license file if present; otherwise treat as private / all rights reserved by the project owner.
