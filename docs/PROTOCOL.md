# TCP binary protocol (K-line + indicators)

One TCP connection streams **candlesticks** (main chart) and **MACD/KDJ** values (sub-charts) in a single response. There is no separate request per chart.

## Frame envelope (all messages)

```
offset  size  field
0       1     msg_type   (u8)
1       4     payload_len (u32 LE)
5       N     payload
```

## Client → server: `GetCandles` (type = 2)

```
payload:
  0      1   symbol_len (u8, 1..32)
  1      L   symbol (UTF-8)
  1+L    1   has_before_index (0 = no, 1 = yes)
  2+L    8   before_index (u64 LE, ignored if has_before_index = 0)
  10+L   4   limit (u32 LE, must be > 0)
```

Without `before_index`, server returns the **latest** `limit` bars.

## Server → client: `CandleChunk` (type = 102)

```
payload:
  0      8   start_index (u64 LE) — file index of first bar in this chunk
  8      8   total (u64 LE)     — total bars in the symbol file
  16     …   body

body layout (two blocks, same bar order):
  [ candle records: N × 32 bytes ]
  [ indicator packs: N × 24 bytes ]

N = body.len() / 56
```

### Candle record (32 bytes, LE i32 × 8)

| offset | field   | notes                          |
|--------|---------|--------------------------------|
| 0      | date    | YYYYMMDD                       |
| 4      | time    | HHMMSS                         |
| 8      | open    | price × 100                    |
| 12     | high    | price × 100                    |
| 16     | low     | price × 100                    |
| 20     | close   | price × 100                    |
| 24     | volume  | shares                         |
| 28     | amount  | turnover ÷ 10000 as integer (see `get_5min.py`) |

### Indicator pack (24 bytes, LE i32 × 6)

Same order as `IndicatorPack` in code: `macd_dif`, `macd_dea`, `macd_bar`, `kdj_k`, `kdj_d`, `kdj_j`.

Each value = `round(indicator × 100)`; `0x80000000` (`i32::MIN`) = invalid / warmup.

## Server → client: `Error` (type = 255)

```
payload:
  0   2   text_len (u16 LE)
  2   L   message (UTF-8)
```

## Code map

| Layer | Rust | Qt client |
|-------|------|-----------|
| Frame | `protocol/frame.rs` | `TcpCandle::decodeFrame` |
| Messages | `protocol/messages.rs` | `TcpCandle::decodeCandleChunk` |
| Candle bytes | `protocol/kline_record.rs` | `TcpCandle::decodeRecords` |
| Indicator bytes | `protocol/indicator_pack.rs` | `TcpCandle::decodeIndicators` |
