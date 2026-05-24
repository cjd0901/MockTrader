#pragma once

#include "model/MarketTypes.h"

#include <QVector>
#include <QWidget>

class QListWidget;

class HomePage final : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);

public slots:
    void setStocks(const QVector<StockRow> &stocks);

signals:
    void openStock(const QString &symbol, const QString &displayName);

private:
    QListWidget *m_list = nullptr;
};
