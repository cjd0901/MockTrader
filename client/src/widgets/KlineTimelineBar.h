#pragma once

#include "model/MarketTypes.h"

#include <QVector>
#include <QWidget>

/// 底部固定刻度尺，仅拖拽平移可见窗口（无滑块、无点击跳转、无 K 线背景）
class KlineTimelineBar final : public QWidget
{
    Q_OBJECT

public:
    explicit KlineTimelineBar(QWidget *parent = nullptr);

    void setCandles(const QVector<CandleBar> &candles, int visibleBarCount);
    void setRange(int minValue, int maxValue);
    void setValue(int value);
    int value() const { return m_value; }
    int maximum() const { return m_maxValue; }
    bool isDragging() const { return m_dragging; }

signals:
    void valueChanged(int value);
    void interactionEnded();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    QSize sizeHint() const override;

private:
    QRectF scaleRect() const;
    qreal scrollTravelPx() const;
    void drawFixedScale(QPainter &painter, const QRectF &area) const;
    void applyValue(int value, bool notify);

    QVector<CandleBar> m_candles;
    int m_visibleBarCount = 100;
    int m_minValue = 0;
    int m_maxValue = 0;
    int m_value = 0;

    bool m_dragging = false;
    qreal m_dragPressX = 0.0;
    int m_dragPressValue = 0;
};
