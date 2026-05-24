#pragma once

#include <QHostAddress>
#include <QUrl>

namespace ServerConfig {

QHostAddress defaultHost();
quint16 defaultTcpPort();
quint16 defaultHttpPort();
QUrl httpBaseUrl(const QHostAddress &host, quint16 httpPort);

} // namespace ServerConfig
