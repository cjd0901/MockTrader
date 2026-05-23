#include "KlineTimelineBar.h"

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

void KlineTimelineBar::setCandles(const QVector<CandleBar> &candles, int visibleBarCount)
{
    m_candles = candles;
    m_visibleBarCount = qMax(1, visibleBarCount);
    Q_UNUSED(m_candles);
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
    if (m_candles.isEmpty() || area.width() <= 1) {
        return 1.0;
    }
    const qreal thumbW =
        qMax(20.0, area.width() * static_cast<qreal>(m_visibleBarCount) / static_cast<qreal>(m_candles.size()));
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

    if (m_candles.isEmpty()) {
        return;
    }

    drawFixedScale(painter, scaleRect());
}

void KlineTimelineBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_candles.isEmpty()) {
        return;
    }

    if (!scaleRect().contains(event->position())) {
        return;
    }

    m_dragging = true;
    m_dragPressX = event->position().x();
    m_dragPressValue = m_value;
}

void KlineTimelineBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || m_candles.isEmpty()) {
        return;
    }

    const qreal travel = scrollTravelPx();
    const qreal deltaPx = event->position().x() - m_dragPressX;
    const int span = m_maxValue - m_minValue;
    const int deltaValue =
        span > 0 ? static_cast<int>(std::lround(deltaPx / travel * static_cast<qreal>(span))) : 0;

    applyValue(qBound(m_minValue, m_dragPressValue + deltaValue, m_maxValue), true);
    update();
}

void KlineTimelineBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (m_dragging) {
        m_dragging = false;
        emit interactionEnded();
    }
}

void KlineTimelineBar::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}
