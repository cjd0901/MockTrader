#include "MainWindow.h"

#include "AppBranding.h"
#include "KlineLoadConfig.h"
#include "net/HttpStockClient.h"
#include "net/TcpTradingClient.h"
#include "pages/HomePage.h"
#include "pages/StockDetailPage.h"

#include <QStatusBar>
#include <QTimer>
#include <QUrl>

namespace {

QHostAddress defaultHost()
{
    const QByteArray env = qgetenv("TRADING_TCP_HOST");
    if (!env.isEmpty()) {
        return QHostAddress(QString::fromUtf8(env));
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

QUrl httpBaseUrl(const QHostAddress &host, quint16 port)
{
    const QString hostStr =
        host.protocol() == QAbstractSocket::IPv6Protocol ? QStringLiteral("[%1]").arg(host.toString())
                                                         : host.toString();
    return QUrl(QStringLiteral("http://%1:%2").arg(hostStr).arg(port));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stack(new QStackedWidget(this))
    , m_home(new HomePage())
    , m_detail(new StockDetailPage())
    , m_http(new HttpStockClient(this))
    , m_tcp(new TcpTradingClient(this))
    , m_host(defaultHost())
    , m_tcpPort(defaultTcpPort())
    , m_httpPort(defaultHttpPort())
{
    setCentralWidget(m_stack);
    m_stack->addWidget(m_home);
    m_stack->addWidget(m_detail);
    m_stack->setStyleSheet(QStringLiteral("background-color:#FFFFFF;"));

    setStyleSheet(QStringLiteral("QMainWindow{background-color:#FFFFFF;}"));
    setWindowIcon(AppBranding::applicationIcon());
    setWindowTitle(tr("MockTrader"));

    wireConnections();

    statusBar()->showMessage(
        tr("HTTP %1:%2 · TCP %3:%4")
            .arg(m_host.toString())
            .arg(m_httpPort)
            .arg(m_host.toString())
            .arg(m_tcpPort));

    QTimer::singleShot(0, this, [this] { connectIfNeeded(); });
}

void MainWindow::wireConnections()
{
    connect(m_http, &HttpStockClient::stockListReceived, m_home, &HomePage::setStocks);
    connect(m_http, &HttpStockClient::fetchError, this, [this](const QString &msg) {
        statusBar()->showMessage(tr("股票列表 HTTP 错误: %1").arg(msg), 8000);
    });

    connect(m_tcp, &TcpTradingClient::connectedChanged, this, [this](bool on) {
        if (on) {
            statusBar()->showMessage(
                tr("HTTP %1:%2 · TCP 已连接 %3:%4")
                    .arg(m_host.toString())
                    .arg(m_httpPort)
                    .arg(m_host.toString())
                    .arg(m_tcpPort),
                5000);
        } else {
            statusBar()->showMessage(
                tr("HTTP %1:%2 · TCP 未连接 %3:%4")
                    .arg(m_host.toString())
                    .arg(m_httpPort)
                    .arg(m_host.toString())
                    .arg(m_tcpPort));
        }
    });

    connect(m_tcp, &TcpTradingClient::connectionError, this, [this](const QString &msg) {
        statusBar()->showMessage(tr("TCP 错误: %1").arg(msg), 8000);
    });

    connect(m_home, &HomePage::openStock, this, [this](const QString &symbol, const QString &name) {
        m_detail->setStock(symbol, name);
        m_stack->setCurrentWidget(m_detail);
        m_tcp->requestCandles(symbol, std::nullopt, KlineLoadConfig::InitialBarLimit);
    });

    connect(m_detail, &StockDetailPage::backRequested, this, [this] {
        m_stack->setCurrentWidget(m_home);
    });

    connect(m_detail, &StockDetailPage::needOlderCandles, this,
            [this](const QString &symbol, quint64 beforeIndex) {
                m_tcp->requestCandles(symbol, beforeIndex, KlineLoadConfig::PrefetchBarLimit);
            });

    connect(m_tcp, &TcpTradingClient::candlesReceived, m_detail, &StockDetailPage::mergeCandles);
}

void MainWindow::fetchStockList()
{
    m_http->fetchStockList(httpBaseUrl(m_host, m_httpPort));
}

void MainWindow::connectIfNeeded()
{
    fetchStockList();
    m_tcp->connectToServer(m_host, m_tcpPort);
}
