<div align="center">

<img src="docs/logo.png" alt="MockTrader" width="160" />

# MockTrader

**量化历史回测 · 模拟拆单下单 · K 线买卖点与收益复盘**

[English](README.md)

</div>

---

MockTrader 是一款**本地优先**的量化历史回测软件，面向 A 股等市场的分钟级行情。系统在历史 5 分钟 K 线上回放行情，支持**自定义量化指标与策略**、**模拟自动下单**，并可**将大单拆分为多笔小单**逐步成交；回测结束后可在 **K 线图上查看买入点、卖出点**，并统计**收益与绩效**。

技术栈：**Rust TCP 服务端**（行情、指标、后续仿真引擎）+ **Qt 6 桌面客户端**（图表、交互、结果展示）。

## 界面截图

![MockTrader 桌面客户端](docs/example.png)

## 功能亮点

| 能力 | 说明 |
|------|------|
| **策略回测** | 在历史 K 线上按 bar 推进，执行自定义量化规则。 |
| **模拟下单** | 根据信号在对应 K 线时刻模拟买入/卖出。 |
| **大单拆小单** | 模拟母单按时间或成交量拆成多笔子单成交。 |
| **图表复盘** | 蜡烛图 + MACD / KDJ；在 K 线上标注买卖点。 |
| **收益分析** | 查看单笔盈亏、胜率、资金曲线等（界面规划中）。 |
| **本地数据** | 行情以二进制文件落地，无需依赖云端。 |

## 开发状态

| 模块 | 状态 |
|------|------|
| 5 分钟 K 线存储与 TCP 接口 | **已实现** |
| 蜡烛图、时间轴拖动、向左预加载 | **已实现** |
| MACD / KDJ（服务端计算，24 字节指标包，i32×100） | **已实现** |
| 策略引擎、下单仿真、拆单逻辑 | **规划中** |
| K 线买卖点标记与收益面板 | **规划中** |

> 本文档描述**产品目标**；表中 **已实现** 为当前版本能力，交易仿真与回测报表仍在迭代中。

## 项目结构

```
MockTrader/
├── crates/src/             # Rust 服务端
│   ├── api/                # HTTP + TCP
│   ├── kline/              # .bin 读写、回测窗口
│   ├── protocol/           # TCP 协议（见 docs/PROTOCOL.md）
│   ├── strategy/           # 回测策略
│   └── indicators/         # MACD / KDJ
├── client/src/             # Qt 6 客户端
│   ├── app/                # 主题、ServerConfig、加载配置
│   ├── api/                # HTTP / TCP 客户端
│   ├── protocol/           # TCP 解码（与服务端一致）
│   ├── pages/              # 首页、详情页
│   └── widgets/            # 图表、回测面板
├── docs/
└── data/
    ├── config/strategies.toml
    ├── scripts/
    └── kline/5min/
```

```mermaid
flowchart LR
  subgraph client [Qt 客户端]
    UI[图表与交互]
    BT[回测界面 - 规划中]
  end
  subgraph server [Rust 服务端]
    IO[K 线读取]
    IND[指标 MACD/KDJ]
    SIM[下单仿真 - 规划中]
  end
  BIN[(.bin 文件)]
  UI -->|HTTP JSON| IO
  UI -->|TCP 二进制| IO
  IO --> BIN
  IND --> IO
  SIM --> IO
  BT --> UI
```

## 准备 K 线数据

依赖 Python 3、`baostock`、`pandas`。

```bash
pip install baostock pandas
python data/scripts/get_5min.py
```

生成示例：`data/kline/5min/立讯精密_002475.bin`。

### 二进制格式（每条 32 字节，小端 int32×8）

| 字段 | 说明 |
|------|------|
| date | `YYYYMMDD` |
| time | `HHmmss`（由 baostock 时间串第 8–14 位得到，如 `093500`） |
| open / high / low / close | 价格 ×100 的整数 |
| volume | 成交量 |
| amount | 成交额 ÷10000 的整数 |

文件名：`{名称}_{代码}.bin`。

## 运行服务端

在项目根目录：

```bash
cargo run -p mock-trader
```

| 环境变量 | 默认值 | 说明 |
|----------|--------|------|
| `TRADING_HOST` | `0.0.0.0` | TCP 监听（K 线/指标） |
| `TRADING_PORT` | `9000` | TCP 端口 |
| `TRADING_HTTP_HOST` | `0.0.0.0` | HTTP 监听（股票列表） |
| `TRADING_HTTP_PORT` | `9080` | HTTP 端口 |
| `KLINE_DIR` | `data/kline/5min` | K 线目录 |

## 构建并运行客户端

需要 Qt **6.5+**（Core、Gui、Widgets、Network、Charts）。

```bash
cd client
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build -j
./build/mock_trader_client
```

| 环境变量 | 默认值 | 说明 |
|----------|--------|------|
| `TRADING_HTTP_HOST` | `127.0.0.1` | HTTP 股票列表 |
| `TRADING_HTTP_PORT` | `9080` | HTTP 端口 |
| `TRADING_TCP_HOST` | `127.0.0.1` | TCP K 线 |
| `TRADING_TCP_PORT` | `9000` | TCP 端口 |

## 客户端使用（当前版本）

1. 启动服务端后打开客户端，首页通过 **HTTP**（`GET /api/stocks`）加载股票列表。
2. 进入详情后通过 **TCP** 加载最近约 **180 个交易日** 的 5 分钟线，默认显示**最新 100 根** K 线。
3. 拖动底部**时间轴**查看更早数据；接近已加载左边界时自动预取约 30 个交易日。
4. **MACD**（DIF、DEA、柱）与 **KDJ** 由服务端计算；顶部读数颜色与图中折线/柱一致。
5. 主图双击显示 OHLC；可见区间最高/最低价标注随滚动更新。
6. 主图右侧 **量化回测** 面板：选择策略（如 MACD 金叉买 / 死叉卖）、在已加载数据的时间范围内设定起止时间并执行。服务端对 `.bin` 文件二分定位该区间并返回买卖点，主图以三角/方块标注。

加载条数可在 `client/src/KlineLoadConfig.h` 调整（`VisibleBarCount`、`InitialBarLimit` 等）。

## HTTP 接口

### 股票列表

`GET /api/stocks` → JSON：

```json
{
  "stocks": [
    { "symbol": "002475", "displayName": "立讯精密" }
  ]
}
```

### 量化策略列表

`GET /api/strategies` → JSON（由 `data/config/strategies.toml` 配置，仅返回已启用且服务端已实现的策略）：

```json
{
  "strategies": [
    { "id": "macd_cross", "displayName": "MACD金叉买入 / 死叉卖出" }
  ]
}
```

配置文件路径可通过环境变量 `STRATEGIES_FILE` 指定（默认 `data/config/strategies.toml`）：

```toml
[[strategy]]
id = "macd_cross"
name = "MACD金叉买入 / 死叉卖出"
enabled = true
```

### K 线文件时间范围

`GET /api/kline/range?symbol=002475` → JSON：

```json
{ "minTs": 1704159000, "maxTs": 1710000000, "totalBars": 12000 }
```

`minTs` / `maxTs` 由 `.bin` 文件首尾记录解析得到（回测时间选择与二分定位的边界）。

### 量化回测

`POST /api/backtest`，请求体示例：

```json
{
  "symbol": "002475",
  "strategy": "macd_cross",
  "startTs": 1704159000,
  "endTs": 1704245400
}
```

响应示例：

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

`summary` 按做多、全仓买卖模拟（期初 10 万）；若期末仍持仓则按区间最后一根收盘价估算。策略：`macd_cross`（DIF 上穿 DEA 买入，下穿卖出）。

## TCP 协议（K 线 / 指标）

帧结构：`[u8 消息类型][u32 小端 payload 长度][payload]`

| 类型 | 值 | 方向 | 说明 |
|------|-----|------|------|
| GetCandles | 2 | C→S | symbol + 可选 `before_index` + `limit` |
| CandleChunk | 102 | S→C | `start_index`、`total`、原始 K 线、指标字节 |
| Error | 255 | S→C | 错误文本 |

每根 K 线：**32 字节行情** + **24 字节指标**（6×i32 LE，值为 round(指标×100)；`i32::MIN` 表示无效：`macd_dif`、`macd_dea`、`macd_bar`、`kdj_k`、`kdj_d`、`kdj_j`）。

`GetCandles` 未带 `before_index` 时返回文件末尾（最新）的 `limit` 条记录。

详见 `docs/PROTOCOL.md`、`crates/src/protocol/`。

## 开发说明

```bash
cargo test -p mock-trader
cargo clean
rm -rf client/build
```

CI：`.github/workflows/mock_trader_server.yml`、`mock_trader_client.yml`。

`.gitignore` 已忽略 `target/`、`client/build/`、本地 `*.bin` 等。

## 许可

以仓库内 LICENSE 为准；若无则归项目作者所有。
