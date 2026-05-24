#include "KlineTimelineBar.h"

#include "app/KlineLoadConfig.h"

#include <QMouseEvent>
#include <QPainter>

#include <cmath>

namespace {
constexpr int kPreferredHeight = 28;
constexpr qreal kHorizontalPad = 10.0;
constexpr qreal kScaleTop = 2.0;
constexpr qreal kBaselineY = 6.0;
constexpr qreal kMinorTickLen = 5.0;
constexpr qreal kMajorTickLen = 10.0;
constexpr qreal kMinorTickStepPx = 6.0;
constexpr int kMinorsPerMajor = 8;
} // namespace

KlineTimelineBar::KlineTimelineBar(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(kPreferredHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::SizeHorCursor);
}

void KlineTimelineBar::setVisibleBarCount(int visibleBarCount)
{
    m_visibleBarCount = qMax(1, visibleBarCount);
    update();
}

void KlineTimelineBar::setRange(int minValue, int maxValue)
{
    m_minValue = minValue;
    m_maxValue = qMax(minValue, maxValue);
    if (m_value < m_minValue) {
        applyValue(m_minValue, false);
    } else if (m_value > m_maxValue) {
        applyValue(m_maxValue, false);
    }
    update();
}

void KlineTimelineBar::setValue(int value)
{
    applyValue(value, false);
    update();
}

void KlineTimelineBar::applyValue(int value, bool notify)
{
    const int clamped = qBound(m_minValue, value, m_maxValue);
    if (clamped == m_value) {
        return;
    }
    m_value = clamped;
    if (notify) {
        emit valueChanged(m_value);
    }
}

QRectF KlineTimelineBar::scaleRect() const
{
    return QRectF(kHorizontalPad, kScaleTop, width() - 2 * kHorizontalPad, kPreferredHeight - kScaleTop - 2);
}

qreal KlineTimelineBar::scrollTravelPx() const
{
    const QRectF area = scaleRect();
    const int span = m_maxValue - m_minValue;
    if (span <= 0 || area.width() <= 1.0) {
        return qMax(1.0, area.width());
    }
    const int totalBars = span + m_visibleBarCount;
    const qreal thumbW =
        qMax(20.0, area.width() * static_cast<qreal>(m_visibleBarCount) / static_cast<qreal>(totalBars));
    return qMax(1.0, area.width() - thumbW);
}

void KlineTimelineBar::drawFixedScale(QPainter &painter, const QRectF &area) const
{
    const qreal baseline = kBaselineY;
    const qreal left = area.left();
    const qreal right = area.right();

    painter.setPen(QPen(QColor(0xD8, 0xD8, 0xD8), 1.0));
    painter.drawLine(QPointF(left, baseline), QPointF(right, baseline));

    int tickIndex = 0;
    for (qreal x = left; x <= right + 0.5; x += kMinorTickStepPx, ++tickIndex) {
        const bool major = (tickIndex % kMinorsPerMajor) == 0;
        const qreal len = major ? kMajorTickLen : kMinorTickLen;
        const qreal penW = major ? 1.8 : 1.0;
        painter.setPen(QPen(major ? QColor(0x88, 0x88, 0x88) : QColor(0xC8, 0xC8, 0xC8), penW));
        painter.drawLine(QPointF(x, baseline), QPointF(x, baseline + len));
    }
}

QSize KlineTimelineBar::sizeHint() const
{
    return {400, kPreferredHeight};
}

void KlineTimelineBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.fillRect(rect(), QColor(0xFF, 0xFF, 0xFF));

    if (m_maxValue <= m_minValue) {
        return;
    }

    drawFixedScale(painter, scaleRect());
}

void KlineTimelineBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_maxValue <= m_minValue) {
        return;
    }

    if (!scaleRect().contains(event->position())) {
        return;
    }

    m_dragging = true;
    m_lastDragX = event->position().x();
    m_dragValueAccum = 0.0;
    grabMouse();
}

void KlineTimelineBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || m_maxValue <= m_minValue) {
        return;
    }

    const qreal travel = scrollTravelPx();
    const qreal x = event->position().x();
    const qreal deltaPx = x - m_lastDragX;
    m_lastDragX = x;

    const int span = m_maxValue - m_minValue;
    if (span <= 0 || qAbs(deltaPx) < 0.25) {
        return;
    }

    m_dragValueAccum +=
        deltaPx / (travel * KlineLoadConfig::TimelineDragScale) * static_cast<qreal>(span);
    const int step = static_cast<int>(m_dragValueAccum);
    if (step == 0) {
        return;
    }
    m_dragValueAccum -= static_cast<qreal>(step);

    applyValue(qBound(m_minValue, m_value + step, m_maxValue), true);
    update();
}

void KlineTimelineBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (m_dragging) {
        m_dragging = false;
        releaseMouse();
        emit interactionEnded();
    }
}

void KlineTimelineBar::wheelEvent(QWheelEvent *event)
{
    if (m_maxValue <= m_minValue) {
        event->ignore();
        return;
    }
    const int wheelStep = qMax(1, static_cast<int>(5.0 / KlineLoadConfig::TimelineDragScale));
    const int delta = event->angleDelta().y() > 0 ? -wheelStep : wheelStep;
    applyValue(qBound(m_minValue, m_value + delta, m_maxValue), true);
    update();
    event->accept();
}
