#include "api/HttpBacktestClient.h"

#include "api/HttpApiUrl.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

HttpBacktestClient::HttpBacktestClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

HttpBacktestClient::~HttpBacktestClient() = default;

void HttpBacktestClient::runBacktest(const QUrl &baseUrl, const QString &symbol,
                                     const QString &strategy, qint64 startTs, qint64 endTs)
{
    const QUrl endpoint = HttpApiUrl::resolve(baseUrl, QStringLiteral("/api/backtest"));
    if (!endpoint.isValid()) {
        emit backtestError(tr("无效的 HTTP 地址"));
        return;
    }

    QJsonObject body;
    body.insert(QStringLiteral("symbol"), symbol);
    body.insert(QStringLiteral("strategy"), strategy);
    body.insert(QStringLiteral("startTs"), startTs);
    body.insert(QStringLiteral("endTs"), endTs);

    QNetworkRequest req(endpoint);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("MockTraderClient/1.0"));

    QNetworkReply *reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit backtestError(reply->errorString());
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit backtestError(tr("回测响应解析失败: %1").arg(parseError.errorString()));
            return;
        }

        const QJsonObject root = doc.object();
        const QJsonValue signalsValue = root.value(QStringLiteral("signals"));
        if (!signalsValue.isArray()) {
            emit backtestError(tr("回测响应缺少 signals"));
            return;
        }

        const QJsonObject summaryObj = root.value(QStringLiteral("summary")).toObject();
        if (summaryObj.isEmpty()) {
            emit backtestError(tr("回测响应缺少 summary"));
            return;
        }

        BacktestSummary summary;
        summary.initialCapital = summaryObj.value(QStringLiteral("initialCapital")).toDouble(100000.0);
        summary.finalEquity = summaryObj.value(QStringLiteral("finalEquity")).toDouble();
        summary.totalReturnPct = summaryObj.value(QStringLiteral("totalReturnPct")).toDouble();
        summary.roundTrips =
            static_cast<quint32>(summaryObj.value(QStringLiteral("roundTrips")).toDouble());
        summary.winCount = static_cast<quint32>(summaryObj.value(QStringLiteral("winCount")).toDouble());
        summary.lossCount =
            static_cast<quint32>(summaryObj.value(QStringLiteral("lossCount")).toDouble());
        summary.openPosition = summaryObj.value(QStringLiteral("openPosition")).toBool();

        QVector<TradeSignal> out;
        for (const QJsonValue &item : signalsValue.toArray()) {
            if (!item.isObject()) {
                continue;
            }
            const QJsonObject obj = item.toObject();
            TradeSignal s;
            s.barIndex = static_cast<quint64>(obj.value(QStringLiteral("barIndex")).toDouble());
            s.tsSec = obj.value(QStringLiteral("tsSec")).toInteger();
            s.side = obj.value(QStringLiteral("side")).toString();
            s.price = obj.value(QStringLiteral("price")).toDouble();
            if (s.side.isEmpty()) {
                continue;
            }
            out.push_back(std::move(s));
        }

        emit backtestFinished(out, summary);
    });
}
