# MockTrader

本地 5 分钟 K 线查看工具：**Rust TCP 服务端**从二进制文件读取行情，**Qt6 客户端**展示蜡烛图。

```
MockTrader/
├── Cargo.toml              # Rust workspace
├── crates/                 # 服务端 mock-trader
├── client/                 # Qt 客户端 mock_trader_client
└── data/
    ├── scripts/get_5min.py # 从 baostock 生成 .bin
    └── kline/5min/         # K 线数据（*.bin，默认不入库）
```

## 功能概览

| 模块 | 说明 |
|------|------|
| 服务端 | TCP 长连接，按索引分页返回原始 32 字节 K 线 |
| 客户端 | 一屏 100 根蜡烛；底部滚动条浏览；可见区间最高/最低价标注 |
| 配色 | 涨红跌绿（收盘 ≥ 开盘为涨），无黑色描边，影线与实体同色 |

## 准备 K 线数据

依赖 Python 3、`baostock`、`pandas`。

```bash
pip install baostock pandas
python data/scripts/get_5min.py
```

生成路径示例：`data/kline/5min/立讯精密_002475.bin`。

### 二进制格式（每条 32 字节，小端 int32×8）

| 字段 | 说明 |
|------|------|
| date | `YYYYMMDD` |
| time | `HHmmss`（由 baostock 时间串第 8–14 位得到，如 `093500`） |
| open / high / low / close | 价格 ×100 的整数 |
| volume | 成交量 |
| amount | 成交额 ÷10000 的整数 |

文件名：`{名称}_{代码}.bin`。

> 若曾用旧版脚本（`time` 使用 `slice(-6)`），请重新执行 `get_5min.py` 覆盖数据。

## 运行服务端

在项目根目录：

```bash
cargo run -p mock-trader
```

| 环境变量 | 默认值 |
|----------|--------|
| `TRADING_HOST` | `0.0.0.0` |
| `TRADING_PORT` | `9000` |
| `KLINE_DIR` | `data/kline/5min` |

## 构建并运行客户端

需要 Qt **6.5+**（Core、Gui、Widgets、Network、Charts）。

```bash
cd client
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build -j
./build/mock_trader_client
```

| 环境变量 | 默认值 |
|----------|--------|
| `TRADING_TCP_HOST` | `127.0.0.1` |
| `TRADING_TCP_PORT` | `9000` |

## 客户端使用说明

1. 启动服务端后打开客户端，首页列出 `KLINE_DIR` 下所有 `.bin` 股票。
2. 进入详情后加载最近约 **180 个交易日** 的 5 分钟线（内存中），图表默认显示**最新 100 根**。
3. 拖动图表**下方滚动条**查看更早 K 线；接近已加载左边界时自动再请求约 30 个交易日数据。
4. 当前可见区间内**最高价**（红字）、**最低价**（绿字）标注在对应蜡烛旁，随滚动更新。

加载条数可在 `client/src/KlineLoadConfig.h` 调整（`VisibleBarCount`、`InitialBarLimit` 等）。

## TCP 协议（二进制帧）

帧结构：`[u8 消息类型][u32 小端 payload 长度][payload]`

| 类型 | 值 | 方向 | 说明 |
|------|-----|------|------|
| ListStocks | 1 | C→S | 无 payload |
| GetCandles | 2 | C→S | symbol + 可选 `before_index` + `limit` |
| StockList | 101 | S→C | 股票列表 UTF-8 |
| CandleChunk | 102 | S→C | `start_index`、`total`、**N×32 字节原始 K 线** |
| Error | 255 | S→C | 错误文本 |

`GetCandles` 未带 `before_index` 时返回文件末尾（最新）的 `limit` 条记录。

协议细节见 `crates/src/protocol/binary.rs`。

## 开发说明

```bash
# 服务端单元测试
cargo test -p mock-trader

# 清理构建产物
cargo clean
rm -rf client/build
```

`.gitignore` 已忽略 `target/`、`client/build/`、`.idea/`、本地 `*.bin` 与数据库文件。
