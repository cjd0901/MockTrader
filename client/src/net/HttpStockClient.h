#pragma once

#include "KlineBinary.h"

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

signals:
    void stockListReceived(const QVector<StockRow> &stocks);
    void fetchError(const QString &message);

private:
    QNetworkAccessManager *m_nam = nullptr;
};
