#include "HttpStockClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

QUrl stocksEndpoint(const QUrl &baseUrl)
{
    QUrl url = baseUrl;
    if (!url.isValid()) {
        return {};
    }
    QString path = url.path();
    if (path.isEmpty() || path == QStringLiteral("/")) {
        url.setPath(QStringLiteral("/api/stocks"));
    } else if (!path.endsWith(QStringLiteral("/api/stocks"))) {
        if (!path.endsWith(QLatin1Char('/'))) {
            path += QLatin1Char('/');
        }
        path += QStringLiteral("api/stocks");
        url.setPath(path);
    }
    return url;
}

} // namespace

HttpStockClient::HttpStockClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

HttpStockClient::~HttpStockClient() = default;

void HttpStockClient::fetchStockList(const QUrl &baseUrl)
{
    const QUrl endpoint = stocksEndpoint(baseUrl);
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
