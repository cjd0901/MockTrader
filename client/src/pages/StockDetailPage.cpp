#include "StockDetailPage.h"

#include "KlineLoadConfig.h"

#include <QtCharts/QCandlestickSet>

#include <QFont>
#include <QGraphicsTextItem>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>
#include <limits>

namespace {
constexpr double kPriceEps = 0.0005;

const QColor kEastMoneyUp{0xF0, 0x3E, 0x3E};
const QColor kEastMoneyDown{0x1B, 0xAA, 0x3A};
const QColor kLabelHighColor{0xF0, 0x3E, 0x3E};
const QColor kLabelLowColor{0x1B, 0xAA, 0x3A};

QGraphicsTextItem *makeExtremaLabel(const QString &text, const QColor &color, QChart *chart)
{
    auto *item = new QGraphicsTextItem(text, chart);
    item->setZValue(100);
    QFont font = item->font();
    font.setPointSize(9);
    font.setBold(true);
    item->setFont(font);
    item->setDefaultTextColor(color);
    return item;
}

void styleCandleSeries(QCandlestickSeries *series, const QColor &color)
{
    series->setIncreasingColor(color);
    series->setDecreasingColor(color);
    series->setPen(QPen(color, 1.0));
    series->setBrush(QBrush(color));
    series->setBodyOutlineVisible(false);
    series->setBodyWidth(0.72);
    // 关闭 cap（最高/最低价处的横线），影线才是竖直线，否则顶端会像左右凸起
    series->setCapsVisible(false);
}
} // namespace

StockDetailPage::StockDetailPage(QWidget *parent)
    : QWidget(parent)
    , m_title(new QLabel(this))
    , m_back(new QPushButton(tr("返回"), this))
    , m_scrollBar(new QScrollBar(Qt::Horizontal, this))
    , m_chart(new QChart())
    , m_view(new QChartView(m_chart))
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_seriesUp(new QCandlestickSeries())
    , m_seriesDown(new QCandlestickSeries())
    , m_visibleBarCount(KlineLoadConfig::VisibleBarCount)
{
    m_chart->legend()->hide();
    m_chart->setAnimationOptions(QChart::NoAnimation);
    m_chart->setMargins(QMargins(8, 8, 8, 4));
    m_chart->setBackgroundBrush(QBrush(QColor(0xFF, 0xFF, 0xFF)));
    m_chart->setPlotAreaBackgroundBrush(QBrush(QColor(0xFF, 0xFF, 0xFF)));

    styleCandleSeries(m_seriesUp, kEastMoneyUp);
    styleCandleSeries(m_seriesDown, kEastMoneyDown);

    m_chart->addSeries(m_seriesUp);
    m_chart->addSeries(m_seriesDown);

    m_axisX->setTitleVisible(false);
    m_axisX->setLabelsVisible(false);
    m_axisX->setGridLineVisible(true);
    m_axisX->setGridLineColor(QColor(0xE8, 0xE8, 0xE8));

    m_axisY->setTitleText(tr("价格"));
    m_axisY->setLabelsColor(QColor(0x66, 0x66, 0x66));
    m_axisY->setGridLineColor(QColor(0xE8, 0xE8, 0xE8));

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    for (QCandlestickSeries *s : {m_seriesUp, m_seriesDown}) {
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisY);
    }

    m_labelHigh = makeExtremaLabel(QString(), kLabelHighColor, m_chart);
    m_labelLow = makeExtremaLabel(QString(), kLabelLowColor, m_chart);
    m_labelHigh->hide();
    m_labelLow->hide();

    m_view->setRenderHint(QPainter::Antialiasing, false);
    m_view->setDragMode(QGraphicsView::NoDrag);

    m_scrollBar->setTracking(true);

    auto *top = new QHBoxLayout();
    top->addWidget(m_back, 0, Qt::AlignLeft);
    top->addStretch(1);
    top->addWidget(new QLabel(tr("5分钟"), this), 0, Qt::AlignRight);

    auto *root = new QVBoxLayout(this);
    root->addLayout(top);
    root->addWidget(m_title);
    root->addWidget(m_view, 1);
    root->addWidget(m_scrollBar, 0);

    connect(m_back, &QPushButton::clicked, this, &StockDetailPage::backRequested);
    connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int) {
        if (m_updatingScroll || m_candles.isEmpty()) {
            return;
        }
        renderVisibleWindow();
        checkPrefetch();
    });
    connect(m_chart, &QChart::plotAreaChanged, this, &StockDetailPage::updateExtremaLabels);
}

void StockDetailPage::setStock(const QString &symbol, const QString &displayName)
{
    m_symbol = symbol;
    m_displayName = displayName;
    m_title->setText(QStringLiteral("%1 (%2) · 5分钟").arg(displayName, symbol));
    resetCandles();
}

void StockDetailPage::resetCandles()
{
    m_candles.clear();
    m_oldestLoadedIndex = 0;
    m_totalCount = 0;
    m_prefetchInFlight = false;
    m_extremaLocalHighIdx = -1;
    m_extremaLocalLowIdx = -1;
    m_seriesUp->clear();
    m_seriesDown->clear();
    m_labelHigh->hide();
    m_labelLow->hide();
    m_updatingScroll = true;
    m_scrollBar->setRange(0, 0);
    m_scrollBar->setValue(0);
    m_updatingScroll = false;
}

void StockDetailPage::mergeCandles(const QString &symbol, quint64 startIndex, quint64 total,
                                 const QVector<CandleBar> &candles)
{
    if (symbol != m_symbol || candles.isEmpty()) {
        return;
    }

    m_prefetchInFlight = false;
    m_totalCount = total;

    const bool firstLoad = m_candles.isEmpty();
    const int scrollBefore = m_scrollBar->value();

    if (firstLoad) {
        m_candles = candles;
        m_oldestLoadedIndex = startIndex;
    } else if (startIndex < m_oldestLoadedIndex) {
        const int added = static_cast<int>(candles.size());
        QVector<CandleBar> merged;
        merged.reserve(added + m_candles.size());
        merged += candles;
        merged += m_candles;
        m_candles = std::move(merged);
        m_oldestLoadedIndex = startIndex;

        syncScrollBarRange();
        m_updatingScroll = true;
        m_scrollBar->setValue(scrollBefore + added);
        m_updatingScroll = false;

        renderVisibleWindow();
        return;
    } else {
        return;
    }

    syncScrollBarRange();
    scrollToLatest();
    renderVisibleWindow();
}

void StockDetailPage::appendCandle(const CandleBar &b, int localIndex)
{
    auto *set = new QCandlestickSet(b.open, b.high, b.low, b.close,
                                    static_cast<qreal>(localIndex));
    if (b.close >= b.open - kPriceEps) {
        m_seriesUp->append(set);
    } else {
        m_seriesDown->append(set);
    }
}

void StockDetailPage::renderVisibleWindow()
{
    m_seriesUp->clear();
    m_seriesDown->clear();
    m_extremaLocalHighIdx = -1;
    m_extremaLocalLowIdx = -1;

    if (m_candles.isEmpty()) {
        m_labelHigh->hide();
        m_labelLow->hide();
        return;
    }

    const int start = m_scrollBar->value();
    const int end = qMin(start + m_visibleBarCount, m_candles.size());
    const int count = end - start;
    if (count <= 0) {
        m_labelHigh->hide();
        m_labelLow->hide();
        return;
    }

    double minY = std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    double rangeHigh = -std::numeric_limits<double>::infinity();
    double rangeLow = std::numeric_limits<double>::infinity();

    for (int i = start; i < end; ++i) {
        const CandleBar &b = m_candles[i];
        const int localIdx = i - start;
        appendCandle(b, localIdx);

        minY = qMin(minY, b.low);
        maxY = qMax(maxY, b.high);

        if (b.high >= rangeHigh) {
            rangeHigh = b.high;
            m_extremaLocalHighIdx = localIdx;
            m_extremaHigh = b.high;
        }
        if (b.low <= rangeLow) {
            rangeLow = b.low;
            m_extremaLocalLowIdx = localIdx;
            m_extremaLow = b.low;
        }
    }

    m_axisX->setRange(-0.5, static_cast<qreal>(count) - 0.5);
    if (std::isfinite(minY) && std::isfinite(maxY) && minY < maxY) {
        const double pad = (maxY - minY) * 0.05;
        m_axisY->setRange(minY - pad, maxY + pad);
    }

    QTimer::singleShot(0, this, &StockDetailPage::updateExtremaLabels);
}

void StockDetailPage::placeLabelBesideCandle(QGraphicsTextItem *label, const QPointF &anchor,
                                             bool preferLeft)
{
    const QRectF bounds = label->boundingRect();
    const qreal gap = 4.0;
    qreal x = preferLeft ? anchor.x() - bounds.width() - gap : anchor.x() + gap;
    qreal y = anchor.y() - bounds.height() / 2.0;

    const QRectF plot = m_chart->plotArea();
    x = qBound(plot.left(), x, plot.right() - bounds.width());
    y = qBound(plot.top(), y, plot.bottom() - bounds.height());
    label->setPos(x, y);
}

void StockDetailPage::updateExtremaLabels()
{
    if (m_extremaLocalHighIdx < 0 || m_extremaLocalLowIdx < 0
        || (m_seriesUp->count() == 0 && m_seriesDown->count() == 0)) {
        m_labelHigh->hide();
        m_labelLow->hide();
        return;
    }

    const QPointF highAnchor =
        m_chart->mapToPosition(QPointF(m_extremaLocalHighIdx, m_extremaHigh), m_seriesUp);
    const QPointF lowAnchor =
        m_chart->mapToPosition(QPointF(m_extremaLocalLowIdx, m_extremaLow), m_seriesUp);

    const QRectF plot = m_chart->plotArea();
    if (!plot.contains(highAnchor) && !plot.contains(lowAnchor)) {
        m_labelHigh->hide();
        m_labelLow->hide();
        return;
    }

    m_labelHigh->setPlainText(QString::number(m_extremaHigh, 'f', 2));
    m_labelLow->setPlainText(QString::number(m_extremaLow, 'f', 2));

    const bool highPreferLeft = highAnchor.x() > plot.center().x();
    const bool lowPreferLeft = lowAnchor.x() > plot.center().x();
    placeLabelBesideCandle(m_labelHigh, highAnchor, highPreferLeft);
    placeLabelBesideCandle(m_labelLow, lowAnchor, lowPreferLeft);

    m_labelHigh->show();
    m_labelLow->show();
}

void StockDetailPage::syncScrollBarRange()
{
    const int n = m_candles.size();
    const int maxStart = qMax(0, n - m_visibleBarCount);

    m_updatingScroll = true;
    m_scrollBar->setRange(0, maxStart);
    m_scrollBar->setPageStep(qMax(1, m_visibleBarCount / 2));
    m_scrollBar->setSingleStep(qMax(1, m_visibleBarCount / 10));
    if (m_scrollBar->value() > maxStart) {
        m_scrollBar->setValue(maxStart);
    }
    m_updatingScroll = false;
}

void StockDetailPage::scrollToLatest()
{
    const int maxStart = qMax(0, m_candles.size() - m_visibleBarCount);
    m_updatingScroll = true;
    m_scrollBar->setValue(maxStart);
    m_updatingScroll = false;
}

void StockDetailPage::checkPrefetch()
{
    if (m_prefetchInFlight || m_symbol.isEmpty() || m_candles.isEmpty()) {
        return;
    }
    if (m_oldestLoadedIndex == 0) {
        return;
    }

    const int start = m_scrollBar->value();
    const qint64 oldestTs = m_candles.first().tsSec;
    const qint64 newestTs = m_candles.last().tsSec;
    const qint64 loadedSpan = qMax<qint64>(1, newestTs - oldestTs);

    const qint64 bufferSec = loadedSpan * KlineLoadConfig::PrefetchBufferTradingDays
        / qMax(1, KlineLoadConfig::InitialTradingDays);

    const qint64 viewportOldestTs = m_candles[qBound(0, start, m_candles.size() - 1)].tsSec;
    if (viewportOldestTs - oldestTs <= bufferSec) {
        m_prefetchInFlight = true;
        emit needOlderCandles(m_symbol, m_oldestLoadedIndex);
    }
}
