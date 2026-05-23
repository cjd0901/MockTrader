#include "HomePage.h"

#include "widgets/StockListDelegate.h"

#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
    , m_list(new QListWidget(this))
{
    setStyleSheet(QStringLiteral("HomePage { background-color: #F3F4F6; }"));

    auto *listCard = new QFrame(this);
    listCard->setObjectName(QStringLiteral("stockListCard"));
    listCard->setAttribute(Qt::WA_StyledBackground, true);
    listCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    listCard->setStyleSheet(QStringLiteral(
        "#stockListCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #DCDCDC;"
        "  border-radius: 12px;"
        "}"));

    auto *cardLayout = new QVBoxLayout(listCard);
    cardLayout->setContentsMargins(4, 8, 4, 8);

    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setSpacing(2);
    m_list->setItemDelegate(new StockListDelegate(m_list));
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background: transparent;"
        "  border: none;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  border: none;"
        "  padding: 0;"
        "}"
        "QListWidget::item:selected {"
        "  background: transparent;"
        "}"));

    cardLayout->addWidget(m_list);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(8);
    layout->addWidget(listCard, 1);

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
        auto *item = new QListWidgetItem();
        item->setData(Qt::UserRole, r.symbol);
        item->setData(Qt::UserRole + 1, r.displayName);
        item->setSizeHint(QSize(0, 58));
        m_list->addItem(item);
    }
}
