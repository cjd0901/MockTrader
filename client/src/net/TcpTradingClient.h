#pragma once

#include "KlineBinary.h"

#include <QHostAddress>
#include <QObject>

#include <optional>

class QTcpSocket;

class TcpTradingClient final : public QObject
{
    Q_OBJECT

public:
    explicit TcpTradingClient(QObject *parent = nullptr);
    ~TcpTradingClient() override;

    void connectToServer(const QHostAddress &host, quint16 port);
    void requestListStocks();
    void requestCandles(const QString &symbol, std::optional<quint64> beforeIndex, quint32 limit);

signals:
    void connectedChanged(bool connected);
    void connectionError(const QString &message);
    void stockListReceived(const QVector<StockRow> &stocks);
    void candlesReceived(const QString &symbol, quint64 startIndex, quint64 total,
                         const QVector<CandleBar> &candles);

private:
    void sendFrame(const QByteArray &frame);
    void onReadyRead();
    void handleFrame(const QByteArray &frame);

    QTcpSocket *m_socket = nullptr;
    QByteArray m_rxBuffer;
    QString m_pendingCandleSymbol;
};
