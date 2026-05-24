#include "MainWindow.h"

#include "app/AppBranding.h"
#include "app/KlineLoadConfig.h"
#include "app/ServerConfig.h"
#include "api/HttpBacktestClient.h"
#include "api/HttpStockClient.h"
#include "api/TcpCandleClient.h"
#include "pages/HomePage.h"
#include "pages/StockDetailPage.h"

#include <QStatusBar>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stack(new QStackedWidget(this))
    , m_home(new HomePage())
    , m_detail(new StockDetailPage())
    , m_http(new HttpStockClient(this))
    , m_backtestHttp(new HttpBacktestClient(this))
    , m_tcp(new TcpCandleClient(this))
    , m_host(ServerConfig::defaultHost())
    , m_tcpPort(ServerConfig::defaultTcpPort())
    , m_httpPort(ServerConfig::defaultHttpPort())
    , m_httpBaseUrl(ServerConfig::httpBaseUrl(m_host, m_httpPort))
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

    connect(m_tcp, &TcpCandleClient::connectedChanged, this, [this](bool on) {
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

    connect(m_tcp, &TcpCandleClient::connectionError, this, [this](const QString &msg) {
        statusBar()->showMessage(tr("TCP 错误: %1").arg(msg), 8000);
    });

    connect(m_home, &HomePage::openStock, this, [this](const QString &symbol, const QString &name) {
        m_detail->setStock(symbol, name);
        m_stack->setCurrentWidget(m_detail);
        m_http->fetchKlineRange(m_httpBaseUrl, symbol);
        m_tcp->requestCandles(symbol, std::nullopt, KlineLoadConfig::InitialBarLimit);
    });

    connect(m_http, &HttpStockClient::klineRangeReceived, m_detail,
            &StockDetailPage::setKlineFileTimeRange);

    connect(m_detail, &StockDetailPage::backRequested, this, [this] {
        m_stack->setCurrentWidget(m_home);
    });

    connect(m_detail, &StockDetailPage::needOlderCandles, this,
            [this](const QString &symbol, quint64 beforeIndex) {
                m_tcp->requestCandles(symbol, beforeIndex, KlineLoadConfig::PrefetchBarLimit);
            });

    connect(m_tcp, &TcpCandleClient::candlesReceived, m_detail, &StockDetailPage::mergeCandles);

    connect(m_detail, &StockDetailPage::backtestRequested, this,
            [this](const QString &symbol, const QString &strategyId, qint64 startTs, qint64 endTs) {
                m_detail->setBacktestRunning(true);
                m_backtestHttp->runBacktest(m_httpBaseUrl, symbol, strategyId, startTs, endTs);
            });
    connect(m_backtestHttp, &HttpBacktestClient::backtestFinished, m_detail,
            &StockDetailPage::setBacktestSignals);
    connect(m_backtestHttp, &HttpBacktestClient::backtestFinished, this, [this]() {
        m_detail->setBacktestRunning(false);
    });
    connect(m_backtestHttp, &HttpBacktestClient::backtestError, m_detail,
            &StockDetailPage::setBacktestError);
}

void MainWindow::fetchStockList()
{
    m_http->fetchStockList(m_httpBaseUrl);
}

void MainWindow::connectIfNeeded()
{
    fetchStockList();
    m_tcp->connectToServer(m_host, m_tcpPort);
}
