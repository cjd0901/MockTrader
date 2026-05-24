#pragma once

#include "model/MarketTypes.h"

#include <QByteArray>
#include <QString>

#include <optional>

namespace TcpCandle {

// --- Wire sizes (see docs/PROTOCOL.md) ---

constexpr int FrameHeaderSize = 5;
constexpr int KlineRecordSize = 32;
constexpr int IndicatorPackSize = 24;
constexpr int CandleChunkHeaderSize = 16;
constexpr int ChunkBodyBarBytes = KlineRecordSize + IndicatorPackSize;

constexpr qint32 IndicatorInvalid = static_cast<qint32>(0x80000000);
constexpr int IndicatorScale = 100;

constexpr quint8 MsgReqGetCandles = 2;
constexpr quint8 MsgRspCandleChunk = 102;
constexpr quint8 MsgRspError = 255;

/// K-line record field offsets (i32 LE each, price × 100).
namespace KlineOff {
constexpr int Date = 0;
constexpr int Time = 4;
constexpr int Open = 8;
constexpr int High = 12;
constexpr int Low = 16;
constexpr int Close = 20;
constexpr int Volume = 24;
/// Turnover ÷ 10000 as i32 (not price scale).
constexpr int Amount = 28;
} // namespace KlineOff

/// Indicator pack field offsets (i32 LE each, value × 100).
namespace IndOff {
constexpr int MacdDif = 0;
constexpr int MacdDea = 4;
constexpr int MacdBar = 8;
constexpr int KdjK = 12;
constexpr int KdjD = 16;
constexpr int KdjJ = 20;
} // namespace IndOff

struct Frame {
    quint8 msgType = 0;
    QByteArray payload;
};

struct CandleChunk {
    quint64 startIndex = 0;
    quint64 total = 0;
    QByteArray records;
    QByteArray indicators;
};

QByteArray encodeGetCandlesRequest(const QString &symbol, std::optional<quint64> beforeIndex,
                                   quint32 limit);

std::optional<Frame> decodeFrame(const QByteArray &frame);
std::optional<CandleChunk> decodeCandleChunkPayload(const QByteArray &payload);
std::optional<QString> decodeErrorPayload(const QByteArray &payload);

QVector<CandleBar> decodeRecords(const QByteArray &records);
QVector<IndicatorBar> decodeIndicators(const QByteArray &indicators, int barCount);

} // namespace TcpCandle
