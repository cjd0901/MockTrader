#pragma once

#include "model/MarketTypes.h"

#include <QObject>
#include <QUrl>
#include <QVector>

class HttpBacktestClient final : public QObject
{
    Q_OBJECT

public:
    explicit HttpBacktestClient(QObject *parent = nullptr);
    ~HttpBacktestClient() override;

    void runBacktest(const QUrl &baseUrl, const QString &symbol, const QString &strategy,
                     qint64 startTs, qint64 endTs);

signals:
    void backtestFinished(const QVector<TradeSignal> &tradeSignals, const BacktestSummary &summary);
    void backtestError(const QString &message);

private:
    class QNetworkAccessManager *m_nam = nullptr;
};
