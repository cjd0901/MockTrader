#pragma once

#include <QtGlobal>

namespace KlineLoadConfig {

/// A 股交易日约 4 小时，5 分钟 K 线约 48 根/日。
constexpr int BarsPerTradingDay = 48;

constexpr int InitialTradingDays = 180;
/// 每次向左预取约 90 个交易日（约 4320 根 5 分钟 K 线）。
constexpr int PrefetchTradingDays = 90;

/// 图表一屏展示的 K 线根数。
constexpr int VisibleBarCount = 100;

/// 视口左缘 index ≤ 此值时才触发向左预取（越小越需拖近最左才加载）。
constexpr int PrefetchBufferBars = 24;

/// 时间轴拖动灵敏度倍率（越大 = 同样像素拖动滚动的 K 线越少，需拖更远）。
constexpr qreal TimelineDragScale = 3.0;

constexpr quint32 InitialBarLimit =
    static_cast<quint32>(BarsPerTradingDay * InitialTradingDays);
constexpr quint32 PrefetchBarLimit =
    static_cast<quint32>(BarsPerTradingDay * PrefetchTradingDays);

} // namespace KlineLoadConfig
