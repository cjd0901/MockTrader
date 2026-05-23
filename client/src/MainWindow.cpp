#include "MainWindow.h"

#include "KlineLoadConfig.h"
#include "net/TcpTradingClient.h"
#include "pages/HomePage.h"
#include "pages/StockDetailPage.h"

#include <QStatusBar>
#include <QTimer>

namespace {
QHostAddress defaultHost()
{
    const QByteArray env = qgetenv("TRADING_TCP_HOST");
    if (!env.isEmpty()) {
        return QHostAddress(QString::fromUtf8(env));
    }
    return QHostAddress::LocalHost;
}

quint16 defaultPort()
{
    const QByteArray env = qgetenv("TRADING_TCP_PORT");
    if (!env.isEmpty()) {
        return env.toUShort();
    }
    return 9000;
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stack(new QStackedWidget(this))
    , m_home(new HomePage())
    , m_detail(new StockDetailPage())
    , m_tcp(new TcpTradingClient(this))
    , m_host(defaultHost())
    , m_port(defaultPort())
{
    setCentralWidget(m_stack);
    m_stack->addWidget(m_home);
    m_stack->addWidget(m_detail);
    m_stack->setStyleSheet(QStringLiteral("background-color:#FFFFFF;"));

    setStyleSheet(QStringLiteral("QMainWindow{background-color:#FFFFFF;}"));
    setWindowTitle(tr("MockTrader"));

    wireConnections();

    statusBar()->showMessage(
        tr("准备连接: %1:%2").arg(m_host.toString()).arg(m_port));

    QTimer::singleShot(0, this, [this] { connectIfNeeded(); });
}

void MainWindow::wireConnections()
{
    connect(m_tcp, &TcpTradingClient::connectedChanged, this, [this](bool on) {
        statusBar()->showMessage(on ? tr("已连接") : tr("未连接"));
        if (on) {
            m_tcp->requestListStocks();
        }
    });

    connect(m_tcp, &TcpTradingClient::connectionError, this, [this](const QString &msg) {
        statusBar()->showMessage(tr("连接错误: %1").arg(msg), 8000);
    });

    connect(m_tcp, &TcpTradingClient::stockListReceived, m_home, &HomePage::setStocks);

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

void MainWindow::connectIfNeeded()
{
    m_tcp->connectToServer(m_host, m_port);
}
