#include "HomePage.h"

#include <QListWidget>
#include <QVBoxLayout>

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
    , m_list(new QListWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!item) {
            return;
        }
        const QString symbol = item->data(Qt::UserRole).toString();
        const QString name = item->data(Qt::UserRole + 1).toString();
        emit openStock(symbol, name);
    });
}

void HomePage::setStocks(const QVector<StockRow> &stocks)
{
    m_list->clear();
    for (const StockRow &r : stocks) {
        auto *item = new QListWidgetItem(QStringLiteral("%1  %2").arg(r.symbol, r.displayName));
        item->setData(Qt::UserRole, r.symbol);
        item->setData(Qt::UserRole + 1, r.displayName);
        m_list->addItem(item);
    }
}
