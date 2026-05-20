import baostock as bs
import pandas as pd
import struct

# ===================== 配置 =====================
STOCK_CODE = "sz.002475"
STOCK_NAME = "立讯精密"
OUTPUT_FILE = f"data/kline/5min/{STOCK_NAME}_002475.bin"
START_DATE = "1990-12-19"
END_DATE = "2026-05-20"
FREQ = "5"
ADJUSTFLAG = "3"
# ==================================================

def download_and_save_32byte_bin():
    bs.login()
    print("正在拉取 立讯精密 5分钟线...")

    rs = bs.query_history_k_data_plus(
        code=STOCK_CODE,
        fields="date,time,open,high,low,close,volume,amount",
        start_date=START_DATE, end_date=END_DATE,
        frequency=FREQ, adjustflag=ADJUSTFLAG
    )

    data_list = []
    while rs.next() and rs.error_code == "0":
        data_list.append(rs.get_row_data())
    df = pd.DataFrame(data_list, columns=rs.fields)
    bs.logout()

    # ===================== 数据处理 =====================
    # 日期：2020-01-02 → 20200102
    df["date"] = df["date"].str.replace("-", "").astype(int)

    # 时间：20200102093500000 → 取第 8–14 位 HHmmss（093500），勿用 slice(-6)
    df["time"] = df["time"].str.slice(8, 14).astype(int)

    # 价格 ×100
    df["open"]   = (df["open"].astype(float) * 100).astype(int)
    df["high"]   = (df["high"].astype(float) * 100).astype(int)
    df["low"]    = (df["low"].astype(float) * 100).astype(int)
    df["close"]  = (df["close"].astype(float) * 100).astype(int)

    # 成交量
    df["volume"] = df["volume"].astype(int)

    # ✅ 成交额：直接 ÷10000 取整
    df["amount"] = (df["amount"].astype(float) / 10000).astype(int)

    # ===================== 写入 32 字节二进制 =====================
    with open(OUTPUT_FILE, "wb") as f:
        for _, row in df.iterrows():
            buf = struct.pack(
                "<iiiiiiii",  # 8个int32 = 32字节
                row["date"],
                row["time"],
                row["open"],
                row["high"],
                row["low"],
                row["close"],
                row["volume"],
                row["amount"]
            )
            f.write(buf)

    print(f"✅ 生成成功：{OUTPUT_FILE}")
    print(f"📊 总K线数量：{len(df)}")
    print(f"💾 文件大小：{len(df)*32 / 1024 / 1024:.2f} MB")

if __name__ == "__main__":
    download_and_save_32byte_bin()