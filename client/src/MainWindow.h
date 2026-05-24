#pragma once

#include <QHostAddress>
#include <QMainWindow>
#include <QStackedWidget>
#include <QUrl>

class HttpBacktestClient;
class HttpStockClient;
class TcpCandleClient;
class HomePage;
class StockDetailPage;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void wireConnections();
    void connectIfNeeded();
    void fetchStockList();

    QStackedWidget *m_stack = nullptr;
    HomePage *m_home = nullptr;
    StockDetailPage *m_detail = nullptr;
    HttpStockClient *m_http = nullptr;
    HttpBacktestClient *m_backtestHttp = nullptr;
    TcpCandleClient *m_tcp = nullptr;
    QHostAddress m_host;
    quint16 m_tcpPort = 9000;
    quint16 m_httpPort = 9080;
    QUrl m_httpBaseUrl;
};
