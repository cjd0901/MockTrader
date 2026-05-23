#include "KlineBinary.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QTimeZone>
#include <QtEndian>

#include <cmath>
#include <cstring>
#include <optional>

namespace {

QByteArray encodeFrame(quint8 msgType, const QByteArray &payload)
{
    QByteArray frame;
    const quint32 len = static_cast<quint32>(payload.size());
    frame.resize(5 + payload.size());
    frame[0] = static_cast<char>(msgType);
    frame[1] = static_cast<char>(len & 0xFF);
    frame[2] = static_cast<char>((len >> 8) & 0xFF);
    frame[3] = static_cast<char>((len >> 16) & 0xFF);
    frame[4] = static_cast<char>((len >> 24) & 0xFF);
    if (!payload.isEmpty()) {
        std::memcpy(frame.data() + 5, payload.constData(), static_cast<size_t>(payload.size()));
    }
    return frame;
}

qint32 readI32(const char *p)
{
    qint32 v = 0;
    std::memcpy(&v, p, 4);
    return qFromLittleEndian<qint32>(v);
}

qint64 candleUnixTs(qint32 date, qint32 time)
{
    const int y = date / 10000;
    const int m = (date / 100) % 100;
    const int d = date % 100;
    // time 为 HHmmss，例如 93500 表示 09:35:00
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

namespace KlineBinary {

QByteArray encodeListStocksRequest()
{
    return encodeFrame(MsgReqListStocks, {});
}

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
        const quint64 idx = *beforeIndex;
        for (int i = 0; i < 8; ++i) {
            payload.append(static_cast<char>((idx >> (8 * i)) & 0xFF));
        }
    } else {
        payload.append(static_cast<char>(0));
        payload.append(QByteArray(8, '\0'));
    }

    for (int i = 0; i < 4; ++i) {
        payload.append(static_cast<char>((limit >> (8 * i)) & 0xFF));
    }

    return encodeFrame(MsgReqGetCandles, payload);
}

QVector<CandleBar> decodeRecords(const QByteArray &records)
{
    if (records.size() % RecordSize != 0) {
        return {};
    }

    const int count = records.size() / RecordSize;
    QVector<CandleBar> out;
    out.reserve(count);

    const char *base = records.constData();
    for (int i = 0; i < count; ++i) {
        const char *p = base + i * RecordSize;
        const qint32 date = readI32(p);
        const qint32 time = readI32(p + 4);
        const qint32 open = readI32(p + 8);
        const qint32 high = readI32(p + 12);
        const qint32 low = readI32(p + 16);
        const qint32 close = readI32(p + 20);
        const qint32 volume = readI32(p + 24);

        CandleBar b;
        b.tsSec = candleUnixTs(date, time);
        b.open = open / 100.0;
        b.high = high / 100.0;
        b.low = low / 100.0;
        b.close = close / 100.0;
        b.volume = volume;
        out.push_back(b);
    }

    return out;
}

QVector<IndicatorBar> decodeIndicators(const QByteArray &indicators, int barCount)
{
    const int expected = barCount * IndicatorValuesSize;
    if (barCount <= 0 || indicators.size() != expected) {
        return {};
    }

    QVector<IndicatorBar> out;
    out.reserve(barCount);

    auto readF64 = [&](int off) -> double {
        quint64 bits = 0;
        for (int i = 0; i < 8; ++i) {
            bits |= quint64(static_cast<quint8>(indicators[off + i])) << (8 * i);
        }
        double v = 0.0;
        std::memcpy(&v, &bits, 8);
        return v;
    };

    for (int i = 0; i < barCount; ++i) {
        const int base = i * IndicatorValuesSize;
        IndicatorBar b;
        b.macdDif = readF64(base);
        b.macdDea = readF64(base + 8);
        b.kdjK = readF64(base + 16);
        b.kdjD = readF64(base + 24);
        b.kdjJ = readF64(base + 32);
        b.macdDifValid = std::isfinite(b.macdDif);
        b.macdDeaValid = std::isfinite(b.macdDea);
        b.kdjKValid = std::isfinite(b.kdjK);
        b.kdjDValid = std::isfinite(b.kdjD);
        b.kdjJValid = std::isfinite(b.kdjJ);
        out.push_back(b);
    }
    return out;
}

} // namespace KlineBinary
