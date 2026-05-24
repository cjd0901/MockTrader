#include "api/HttpStockClient.h"

#include "api/HttpApiUrl.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

HttpStockClient::HttpStockClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

HttpStockClient::~HttpStockClient() = default;

void HttpStockClient::fetchStockList(const QUrl &baseUrl)
{
    const QUrl endpoint = HttpApiUrl::resolve(baseUrl, QStringLiteral("/api/stocks"));
    if (!endpoint.isValid()) {
        emit fetchError(tr("无效的 HTTP 地址"));
        return;
    }

    QNetworkRequest req(endpoint);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("MockTraderClient/1.0"));

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit fetchError(reply->errorString());
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit fetchError(tr("股票列表 JSON 解析失败: %1").arg(parseError.errorString()));
            return;
        }

        const QJsonObject root = doc.object();
        const QJsonValue stocksValue = root.value(QStringLiteral("stocks"));
        if (!stocksValue.isArray()) {
            emit fetchError(tr("响应缺少 stocks 数组"));
            return;
        }

        const QJsonArray stocks = stocksValue.toArray();
        QVector<StockRow> rows;
        rows.reserve(stocks.size());

        for (const QJsonValue &item : stocks) {
            if (!item.isObject()) {
                continue;
            }
            const QJsonObject obj = item.toObject();
            const QString symbol = obj.value(QStringLiteral("symbol")).toString();
            const QString displayName = obj.value(QStringLiteral("displayName")).toString();
            if (symbol.isEmpty()) {
                continue;
            }
            StockRow row;
            row.symbol = symbol;
            row.displayName = displayName.isEmpty() ? symbol : displayName;
            rows.push_back(std::move(row));
        }

        emit stockListReceived(rows);
    });
}

void HttpStockClient::fetchKlineRange(const QUrl &baseUrl, const QString &symbol)
{
    if (symbol.isEmpty()) {
        emit fetchError(tr("无效的股票代码"));
        return;
    }

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("symbol"), symbol);
    const QUrl endpoint =
        HttpApiUrl::resolve(baseUrl, QStringLiteral("/api/kline/range"), query);
    if (!endpoint.isValid()) {
        emit fetchError(tr("无效的 HTTP 地址"));
        return;
    }

    QNetworkRequest req(endpoint);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("MockTraderClient/1.0"));

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, symbol]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit fetchError(reply->errorString());
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit fetchError(tr("K 线范围 JSON 解析失败: %1").arg(parseError.errorString()));
            return;
        }

        const QJsonObject root = doc.object();
        const qint64 minTs = root.value(QStringLiteral("minTs")).toInteger();
        const qint64 maxTs = root.value(QStringLiteral("maxTs")).toInteger();
        const quint64 totalBars =
            static_cast<quint64>(root.value(QStringLiteral("totalBars")).toDouble());
        if (minTs <= 0 || maxTs < minTs) {
            emit fetchError(tr("K 线时间范围无效"));
            return;
        }

        emit klineRangeReceived(symbol, minTs, maxTs, totalBars);
    });
}
