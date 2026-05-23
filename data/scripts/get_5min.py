import baostock as bs
import pandas as pd
import struct
import toml
from pathlib import Path

START_DATE = "1990-12-19"
END_DATE = "2026-05-20"
FREQ = "5"
ADJUSTFLAG = "3"

def load_stocks_config():
    """加载同目录下的 stocks.toml"""
    config_path = Path(__file__).parent / "stocks.toml"
    with open(config_path, "r", encoding="utf-8") as f:
        config = toml.load(f)
    return config["stock"]

def download_and_save_32byte_bin(stock_code, stock_name):
    output_file = f"data/kline/5min/{stock_name}_{stock_code.split('.')[1]}.bin"

    Path(output_file).parent.mkdir(parents=True, exist_ok=True)

    print(f"正在拉取 {stock_name}({stock_code}) 5分钟线...")

    rs = bs.query_history_k_data_plus(
        code=stock_code,
        fields="date,time,open,high,low,close,volume,amount",
        start_date=START_DATE, end_date=END_DATE,
        frequency=FREQ, adjustflag=ADJUSTFLAG
    )

    data_list = []
    while rs.next() and rs.error_code == "0":
        data_list.append(rs.get_row_data())
    
    if not data_list:
        print(f"❌ {stock_name} 无数据，跳过\n")
        return

    df = pd.DataFrame(data_list, columns=rs.fields)

    df["date"] = df["date"].str.replace("-", "").astype(int)
    df["time"] = df["time"].str.slice(8, 14).astype(int)

    df["open"]   = (df["open"].astype(float) * 100).astype(int)
    df["high"]   = (df["high"].astype(float) * 100).astype(int)
    df["low"]    = (df["low"].astype(float) * 100).astype(int)
    df["close"]  = (df["close"].astype(float) * 100).astype(int)
    df["volume"] = df["volume"].astype(int)
    df["amount"] = (df["amount"].astype(float) / 10000).astype(int)

    with open(output_file, "wb") as f:
        for _, row in df.iterrows():
            buf = struct.pack(
                "<iiiiiiii",
                row["date"], row["time"],
                row["open"], row["high"], row["low"], row["close"],
                row["volume"], row["amount"]
            )
            f.write(buf)

    print(f"✅ {stock_name} 生成成功：{output_file}")
    print(f"📊 K线数量：{len(df)}")
    print(f"💾 文件大小：{len(df)*32 / 1024 / 1024:.2f} MB\n")

if __name__ == "__main__":
    bs.login()
    stocks = load_stocks_config()

    for stock in stocks:
        download_and_save_32byte_bin(stock["code"], stock["name"])

    bs.logout()
    print("🎉 所有股票处理完成！")