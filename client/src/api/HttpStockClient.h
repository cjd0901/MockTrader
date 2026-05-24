#pragma once

#include "model/MarketTypes.h"

#include <QObject>
#include <QUrl>
#include <QVector>

class QNetworkAccessManager;

class HttpStockClient final : public QObject
{
    Q_OBJECT

public:
    explicit HttpStockClient(QObject *parent = nullptr);
    ~HttpStockClient() override;

    void fetchStockList(const QUrl &baseUrl);
    void fetchKlineRange(const QUrl &baseUrl, const QString &symbol);

signals:
    void stockListReceived(const QVector<StockRow> &stocks);
    void klineRangeReceived(const QString &symbol, qint64 minTs, qint64 maxTs, quint64 totalBars);
    void fetchError(const QString &message);

private:
    QNetworkAccessManager *m_nam = nullptr;
};
