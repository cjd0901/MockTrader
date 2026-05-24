#include "api/TcpCandleClient.h"

#include "protocol/TcpCandleCodec.h"

#include <QAbstractSocket>
#include <QTcpSocket>

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

    while (m_rxBuffer.size() >= 5) {
        const auto *hdr = reinterpret_cast<const uchar *>(m_rxBuffer.constData());
        const quint32 len = quint32(hdr[1]) | (quint32(hdr[2]) << 8) | (quint32(hdr[3]) << 16)
            | (quint32(hdr[4]) << 24);
        if (len > 16u * 1024u * 1024u) {
            emit connectionError(tr("服务端帧过大"));
            m_socket->disconnectFromHost();
            return;
        }
        if (static_cast<quint32>(m_rxBuffer.size()) < 5u + len) {
            break;
        }

        const QByteArray frame = m_rxBuffer.mid(0, 5 + static_cast<int>(len));
        m_rxBuffer.remove(0, 5 + static_cast<int>(len));
        handleFrame(frame);
    }
}

void TcpCandleClient::handleFrame(const QByteArray &frame)
{
    if (frame.size() < 5) {
        return;
    }

    const quint8 msgType = static_cast<quint8>(frame[0]);
    const quint32 len = static_cast<quint32>(frame[1]) | (static_cast<quint32>(frame[2]) << 8)
        | (static_cast<quint32>(frame[3]) << 16) | (static_cast<quint32>(frame[4]) << 24);
    const QByteArray payload = frame.mid(5, static_cast<int>(len));

    if (msgType == TcpCandle::MsgRspError) {
        if (payload.size() < 2) {
            emit connectionError(tr("错误帧格式无效"));
            return;
        }
        const quint16 textLen = quint16(static_cast<uchar>(payload[0]))
            | (quint16(static_cast<uchar>(payload[1])) << 8);
        const QString msg = QString::fromUtf8(payload.mid(2, textLen));
        emit connectionError(msg);
        return;
    }

    if (msgType == TcpCandle::MsgRspCandleChunk) {
        if (payload.size() < 16) {
            emit connectionError(tr("K线帧头过短"));
            return;
        }
        const quint64 startIndex = quint64(static_cast<uchar>(payload[0]))
            | (quint64(static_cast<uchar>(payload[1])) << 8)
            | (quint64(static_cast<uchar>(payload[2])) << 16)
            | (quint64(static_cast<uchar>(payload[3])) << 24)
            | (quint64(static_cast<uchar>(payload[4])) << 32)
            | (quint64(static_cast<uchar>(payload[5])) << 40)
            | (quint64(static_cast<uchar>(payload[6])) << 48)
            | (quint64(static_cast<uchar>(payload[7])) << 56);
        const quint64 total = quint64(static_cast<uchar>(payload[8]))
            | (quint64(static_cast<uchar>(payload[9])) << 8)
            | (quint64(static_cast<uchar>(payload[10])) << 16)
            | (quint64(static_cast<uchar>(payload[11])) << 24)
            | (quint64(static_cast<uchar>(payload[12])) << 32)
            | (quint64(static_cast<uchar>(payload[13])) << 40)
            | (quint64(static_cast<uchar>(payload[14])) << 48)
            | (quint64(static_cast<uchar>(payload[15])) << 56);

        const QByteArray body = payload.mid(16);
        const int barBytes = TcpCandle::RecordSize + TcpCandle::IndicatorValuesSize;
        if (!body.isEmpty() && body.size() % barBytes != 0) {
            emit connectionError(tr("K线/指标数据长度无效"));
            return;
        }
        const int barCount = body.size() / barBytes;
        const QByteArray records = body.left(barCount * TcpCandle::RecordSize);
        const QByteArray indicatorBytes = body.mid(barCount * TcpCandle::RecordSize);

        const QVector<CandleBar> candles = TcpCandle::decodeRecords(records);
        if (candles.isEmpty() && !records.isEmpty()) {
            emit connectionError(tr("K线解码失败"));
            return;
        }

        const QVector<IndicatorBar> indicators =
            TcpCandle::decodeIndicators(indicatorBytes, barCount);
        if (indicators.size() != candles.size()) {
            emit connectionError(tr("指标解码失败"));
            return;
        }

        emit candlesReceived(m_pendingCandleSymbol, startIndex, total, candles, indicators);
        return;
    }

    emit connectionError(tr("未知消息类型: %1").arg(msgType));
}
