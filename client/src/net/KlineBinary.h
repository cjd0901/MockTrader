#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

#include <optional>

struct CandleBar
{
    qint64 tsSec = 0;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double volume = 0.0;
};

struct StockRow
{
    QString symbol;
    QString displayName;
};

struct IndicatorBar
{
    double macdDif = 0.0;
    double macdDea = 0.0;
    double macdBar = 0.0;
    double kdjK = 0.0;
    double kdjD = 0.0;
    double kdjJ = 0.0;
    bool macdDifValid = false;
    bool macdDeaValid = false;
    bool macdBarValid = false;
    bool kdjKValid = false;
    bool kdjDValid = false;
    bool kdjJValid = false;
};

namespace KlineBinary {

constexpr int RecordSize = 32;
constexpr int IndicatorValuesSize = 24;
/// Sentinel on wire when an indicator is not available (`i32::MIN`).
constexpr qint32 IndicatorInvalid = static_cast<qint32>(0x80000000);
constexpr int IndicatorScale = 100;

constexpr quint8 MsgReqListStocks = 1;
constexpr quint8 MsgReqGetCandles = 2;
constexpr quint8 MsgRspStockList = 101;
constexpr quint8 MsgRspCandleChunk = 102;
constexpr quint8 MsgRspError = 255;

QByteArray encodeListStocksRequest();
QByteArray encodeGetCandlesRequest(const QString &symbol, std::optional<quint64> beforeIndex,
                                   quint32 limit);

QVector<CandleBar> decodeRecords(const QByteArray &records);
QVector<IndicatorBar> decodeIndicators(const QByteArray &indicators, int barCount);

} // namespace KlineBinary
