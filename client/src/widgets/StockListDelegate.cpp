#include "StockListDelegate.h"

#include <QPainter>

StockListDelegate::StockListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void StockListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString symbol = index.data(Qt::UserRole).toString();
    const QString name = index.data(Qt::UserRole + 1).toString();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const bool selected = opt.state & QStyle::State_Selected;
    const bool hover = opt.state & QStyle::State_MouseOver;

    QRectF card = opt.rect.adjusted(6, 3, -6, -3);
    if (selected) {
        painter->setPen(QPen(QColor(0x42, 0x85, 0xF4), 1.0));
        painter->setBrush(QColor(0xE8, 0xF1, 0xFF));
    } else if (hover) {
        painter->setPen(QPen(QColor(0xE8, 0xE8, 0xE8), 1.0));
        painter->setBrush(QColor(0xF7, 0xF9, 0xFC));
    } else {
        painter->setPen(QPen(QColor(0xEF, 0xEF, 0xEF), 1.0));
        painter->setBrush(QColor(0xFF, 0xFF, 0xFF));
    }
    painter->drawRoundedRect(card, 8, 8);

    const QRectF textRect = card.adjusted(14, 0, -14, 0);

    QFont nameFont = opt.font;
    nameFont.setPointSize(14);
    nameFont.setWeight(QFont::DemiBold);
    painter->setFont(nameFont);
    painter->setPen(QColor(0x22, 0x22, 0x22));
    painter->drawText(QRectF(textRect.left(), textRect.top(), textRect.width(), textRect.height() / 2),
                      Qt::AlignLeft | Qt::AlignVCenter, name);

    QFont symFont = opt.font;
    symFont.setPointSize(12);
    painter->setFont(symFont);
    painter->setPen(QColor(0x88, 0x88, 0x88));
    painter->drawText(
        QRectF(textRect.left(), textRect.center().y(), textRect.width(), textRect.height() / 2),
        Qt::AlignLeft | Qt::AlignVCenter, symbol);

    painter->restore();
}

QSize StockListDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    Q_UNUSED(index);
    return {option.rect.width(), 58};
}
