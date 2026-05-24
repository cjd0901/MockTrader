#pragma once

#include <QtGlobal>

namespace KlineLoadConfig {

/// A 股交易日约 4 小时，5 分钟 K 线约 48 根/日。
constexpr int BarsPerTradingDay = 48;

constexpr int InitialTradingDays = 180;
constexpr int PrefetchTradingDays = 30;
constexpr int PrefetchBufferTradingDays = 30;

/// 图表一屏展示的 K 线根数。
constexpr int VisibleBarCount = 100;

constexpr quint32 InitialBarLimit =
    static_cast<quint32>(BarsPerTradingDay * InitialTradingDays);
constexpr quint32 PrefetchBarLimit =
    static_cast<quint32>(BarsPerTradingDay * PrefetchTradingDays);

} // namespace KlineLoadConfig
