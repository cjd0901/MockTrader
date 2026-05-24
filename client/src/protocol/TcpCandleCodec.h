#pragma once

#include "model/MarketTypes.h"

#include <QByteArray>
#include <QString>

#include <optional>

namespace TcpCandle {

constexpr int RecordSize = 32;
constexpr int IndicatorValuesSize = 24;
constexpr qint32 IndicatorInvalid = static_cast<qint32>(0x80000000);
constexpr int IndicatorScale = 100;

constexpr quint8 MsgReqGetCandles = 2;
constexpr quint8 MsgRspCandleChunk = 102;
constexpr quint8 MsgRspError = 255;

QByteArray encodeGetCandlesRequest(const QString &symbol, std::optional<quint64> beforeIndex,
                                   quint32 limit);

QVector<CandleBar> decodeRecords(const QByteArray &records);
QVector<IndicatorBar> decodeIndicators(const QByteArray &indicators, int barCount);

} // namespace TcpCandle
