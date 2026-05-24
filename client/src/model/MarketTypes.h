#pragma once

#include <QString>
#include <QVector>

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

struct TradeSignal
{
    quint64 barIndex = 0;
    qint64 tsSec = 0;
    QString side;
    double price = 0.0;
};

struct BacktestSummary
{
    double initialCapital = 100000.0;
    double finalEquity = 100000.0;
    double totalReturnPct = 0.0;
    quint32 roundTrips = 0;
    quint32 winCount = 0;
    quint32 lossCount = 0;
    bool openPosition = false;
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
