#include "protocol/TcpCandleCodec.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QTimeZone>
#include <QtEndian>

#include <cstring>
#include <optional>

namespace {

qint32 readI32(const char *p)
{
    qint32 v = 0;
    std::memcpy(&v, p, 4);
    return qFromLittleEndian<qint32>(v);
}

quint32 readU32(const char *p)
{
    quint32 v = 0;
    std::memcpy(&v, p, 4);
    return qFromLittleEndian<quint32>(v);
}

quint64 readU64(const char *p)
{
    quint64 v = 0;
    std::memcpy(&v, p, 8);
    return qFromLittleEndian<quint64>(v);
}

void writeU32(QByteArray &out, quint32 v)
{
    const quint32 le = qToLittleEndian(v);
    out.append(reinterpret_cast<const char *>(&le), 4);
}

void writeU64(QByteArray &out, quint64 v)
{
    const quint64 le = qToLittleEndian(v);
    out.append(reinterpret_cast<const char *>(&le), 8);
}

QByteArray encodeFrame(quint8 msgType, const QByteArray &payload)
{
    QByteArray frame;
    const quint32 len = static_cast<quint32>(payload.size());
    frame.resize(TcpCandle::FrameHeaderSize + payload.size());
    frame[0] = static_cast<char>(msgType);
    const quint32 lenLe = qToLittleEndian(len);
    std::memcpy(frame.data() + 1, &lenLe, 4);
    if (!payload.isEmpty()) {
        std::memcpy(frame.data() + TcpCandle::FrameHeaderSize, payload.constData(),
                    static_cast<size_t>(payload.size()));
    }
    return frame;
}

qint64 candleUnixTs(qint32 date, qint32 time)
{
    const int y = date / 10000;
    const int m = (date / 100) % 100;
    const int d = date % 100;
    const int hh = time / 10000;
    const int mm = (time / 100) % 100;
    const int ss = time % 100;
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59) {
        return 0;
    }

    const QDate qd(y, m, d);
    const QTime qt(hh, mm, ss);
    const QTimeZone tz(QByteArrayLiteral("Asia/Shanghai"));
    const QDateTime dt(qd, qt, tz);
    return dt.toSecsSinceEpoch();
}

} // namespace

namespace TcpCandle {

QByteArray encodeGetCandlesRequest(const QString &symbol, std::optional<quint64> beforeIndex,
                                   quint32 limit)
{
    const QByteArray sym = symbol.toUtf8();
    QByteArray payload;
    const int symLen = qMin(sym.size(), 32);
    payload.reserve(1 + symLen + 13);
    payload.append(static_cast<char>(symLen));
    payload.append(sym.constData(), symLen);

    if (beforeIndex) {
        payload.append(static_cast<char>(1));
        writeU64(payload, *beforeIndex);
    } else {
        payload.append(static_cast<char>(0));
        writeU64(payload, 0);
    }
    writeU32(payload, limit);

    return encodeFrame(MsgReqGetCandles, payload);
}

std::optional<Frame> decodeFrame(const QByteArray &frame)
{
    if (frame.size() < FrameHeaderSize) {
        return std::nullopt;
    }
    const quint32 len = readU32(frame.constData() + 1);
    if (static_cast<quint32>(frame.size()) != FrameHeaderSize + len) {
        return std::nullopt;
    }
    Frame out;
    out.msgType = static_cast<quint8>(frame[0]);
    out.payload = frame.mid(FrameHeaderSize, static_cast<int>(len));
    return out;
}

std::optional<CandleChunk> decodeCandleChunkPayload(const QByteArray &payload)
{
    if (payload.size() < CandleChunkHeaderSize) {
        return std::nullopt;
    }

    const char *base = payload.constData();
    CandleChunk chunk;
    chunk.startIndex = readU64(base);
    chunk.total = readU64(base + 8);

    const QByteArray body = payload.mid(CandleChunkHeaderSize);
    if (body.isEmpty()) {
        return chunk;
    }

    if (body.size() % ChunkBodyBarBytes != 0) {
        return std::nullopt;
    }

    const int barCount = body.size() / ChunkBodyBarBytes;
    const int recordsLen = barCount * KlineRecordSize;
    chunk.records = body.left(recordsLen);
    chunk.indicators = body.mid(recordsLen);
    return chunk;
}

std::optional<QString> decodeErrorPayload(const QByteArray &payload)
{
    if (payload.size() < 2) {
        return std::nullopt;
    }
    quint16 textLen = 0;
    std::memcpy(&textLen, payload.constData(), 2);
    textLen = qFromLittleEndian<quint16>(textLen);
    if (payload.size() < 2 + textLen) {
        return std::nullopt;
    }
    return QString::fromUtf8(payload.mid(2, textLen));
}

QVector<CandleBar> decodeRecords(const QByteArray &records)
{
    if (records.size() % KlineRecordSize != 0) {
        return {};
    }

    const int count = records.size() / KlineRecordSize;
    QVector<CandleBar> out;
    out.reserve(count);

    const char *base = records.constData();
    for (int i = 0; i < count; ++i) {
        const char *p = base + i * KlineRecordSize;
        const qint32 date = readI32(p + KlineOff::Date);
        const qint32 time = readI32(p + KlineOff::Time);

        CandleBar b;
        b.tsSec = candleUnixTs(date, time);
        b.open = readI32(p + KlineOff::Open) / 100.0;
        b.high = readI32(p + KlineOff::High) / 100.0;
        b.low = readI32(p + KlineOff::Low) / 100.0;
        b.close = readI32(p + KlineOff::Close) / 100.0;
        b.volume = readI32(p + KlineOff::Volume);
        out.push_back(b);
    }

    return out;
}

QVector<IndicatorBar> decodeIndicators(const QByteArray &indicators, int barCount)
{
    const int expected = barCount * IndicatorPackSize;
    if (barCount <= 0 || indicators.size() != expected) {
        return {};
    }

    QVector<IndicatorBar> out;
    out.reserve(barCount);

    auto readStored = [&](int off) -> std::pair<double, bool> {
        const qint32 raw = readI32(indicators.constData() + off);
        if (raw == IndicatorInvalid) {
            return {0.0, false};
        }
        return {raw / static_cast<double>(IndicatorScale), true};
    };

    for (int i = 0; i < barCount; ++i) {
        const int base = i * IndicatorPackSize;
        IndicatorBar b;
        std::tie(b.macdDif, b.macdDifValid) = readStored(base + IndOff::MacdDif);
        std::tie(b.macdDea, b.macdDeaValid) = readStored(base + IndOff::MacdDea);
        std::tie(b.macdBar, b.macdBarValid) = readStored(base + IndOff::MacdBar);
        std::tie(b.kdjK, b.kdjKValid) = readStored(base + IndOff::KdjK);
        std::tie(b.kdjD, b.kdjDValid) = readStored(base + IndOff::KdjD);
        std::tie(b.kdjJ, b.kdjJValid) = readStored(base + IndOff::KdjJ);
        out.push_back(b);
    }
    return out;
}

} // namespace TcpCandle
