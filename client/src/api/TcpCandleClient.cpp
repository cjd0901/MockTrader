#include "api/TcpCandleClient.h"

#include "protocol/TcpCandleCodec.h"

#include <QAbstractSocket>
#include <QTcpSocket>
#include <QtEndian>

TcpCandleClient::TcpCandleClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, [this] { emit connectedChanged(true); });
    connect(m_socket, &QTcpSocket::disconnected, this, [this] { emit connectedChanged(false); });
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpCandleClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit connectionError(m_socket->errorString());
    });
}

TcpCandleClient::~TcpCandleClient() = default;

void TcpCandleClient::connectToServer(const QHostAddress &host, quint16 port)
{
    m_rxBuffer.clear();
    m_pendingCandleSymbol.clear();
    m_socket->abort();
    m_socket->connectToHost(host, port);
}

void TcpCandleClient::requestCandles(const QString &symbol, std::optional<quint64> beforeIndex,
                                     quint32 limit)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    m_pendingCandleSymbol = symbol;
    sendFrame(TcpCandle::encodeGetCandlesRequest(symbol, beforeIndex, limit));
}

void TcpCandleClient::sendFrame(const QByteArray &frame)
{
    m_socket->write(frame);
}

void TcpCandleClient::onReadyRead()
{
    m_rxBuffer.append(m_socket->readAll());

    while (m_rxBuffer.size() >= TcpCandle::FrameHeaderSize) {
        const quint32 len = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(m_rxBuffer.constData() + 1));
        if (len > 16u * 1024u * 1024u) {
            emit connectionError(tr("服务端帧过大"));
            m_socket->disconnectFromHost();
            return;
        }
        if (static_cast<quint32>(m_rxBuffer.size()) < TcpCandle::FrameHeaderSize + len) {
            break;
        }

        const QByteArray frame = m_rxBuffer.mid(0, TcpCandle::FrameHeaderSize + static_cast<int>(len));
        m_rxBuffer.remove(0, TcpCandle::FrameHeaderSize + static_cast<int>(len));
        handleFrame(frame);
    }
}

void TcpCandleClient::handleFrame(const QByteArray &frame)
{
    const std::optional<TcpCandle::Frame> decoded = TcpCandle::decodeFrame(frame);
    if (!decoded) {
        emit connectionError(tr("帧格式无效"));
        return;
    }

    if (decoded->msgType == TcpCandle::MsgRspError) {
        const std::optional<QString> msg = TcpCandle::decodeErrorPayload(decoded->payload);
        emit connectionError(msg.value_or(tr("未知服务端错误")));
        return;
    }

    if (decoded->msgType == TcpCandle::MsgRspCandleChunk) {
        const std::optional<TcpCandle::CandleChunk> chunk =
            TcpCandle::decodeCandleChunkPayload(decoded->payload);
        if (!chunk) {
            emit connectionError(tr("K 线块解码失败"));
            return;
        }

        const QVector<CandleBar> candles = TcpCandle::decodeRecords(chunk->records);
        if (candles.isEmpty() && !chunk->records.isEmpty()) {
            emit connectionError(tr("K 线解码失败"));
            return;
        }

        const int barCount = candles.size();
        const QVector<IndicatorBar> indicators =
            TcpCandle::decodeIndicators(chunk->indicators, barCount);
        if (indicators.size() != candles.size()) {
            emit connectionError(tr("指标解码失败"));
            return;
        }

        emit candlesReceived(m_pendingCandleSymbol, chunk->startIndex, chunk->total, candles,
                             indicators);
        return;
    }

    emit connectionError(tr("未知消息类型: %1").arg(decoded->msgType));
}
