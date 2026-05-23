#pragma once

#include <QStyledItemDelegate>

class StockListDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit StockListDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
