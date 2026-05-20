#pragma once

#include <QHostAddress>
#include <QMainWindow>
#include <QStackedWidget>

class TcpTradingClient;
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

    QStackedWidget *m_stack = nullptr;
    HomePage *m_home = nullptr;
    StockDetailPage *m_detail = nullptr;
    TcpTradingClient *m_tcp = nullptr;
    QHostAddress m_host;
    quint16 m_port = 9000;
};
