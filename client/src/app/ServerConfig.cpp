#include "app/ServerConfig.h"

namespace ServerConfig {

QHostAddress defaultHost()
{
    const QByteArray tcpEnv = qgetenv("TRADING_TCP_HOST");
    if (!tcpEnv.isEmpty()) {
        return QHostAddress(QString::fromUtf8(tcpEnv));
    }
    const QByteArray httpEnv = qgetenv("TRADING_HTTP_HOST");
    if (!httpEnv.isEmpty()) {
        return QHostAddress(QString::fromUtf8(httpEnv));
    }
    return QHostAddress::LocalHost;
}

quint16 defaultTcpPort()
{
    const QByteArray env = qgetenv("TRADING_TCP_PORT");
    if (!env.isEmpty()) {
        return env.toUShort();
    }
    return 9000;
}

quint16 defaultHttpPort()
{
    const QByteArray env = qgetenv("TRADING_HTTP_PORT");
    if (!env.isEmpty()) {
        return env.toUShort();
    }
    return 9080;
}

QUrl httpBaseUrl(const QHostAddress &host, quint16 httpPort)
{
    const QString hostStr = host.protocol() == QAbstractSocket::IPv6Protocol
                                ? QStringLiteral("[%1]").arg(host.toString())
                                : host.toString();
    return QUrl(QStringLiteral("http://%1:%2").arg(hostStr).arg(httpPort));
}

} // namespace ServerConfig
