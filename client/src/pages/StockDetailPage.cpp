#include "StockDetailPage.h"

#include "KlineLoadConfig.h"
#include "widgets/KlineTimelineBar.h"

#include <QtCharts/QCandlestickSet>

#include <QDateTime>
#include <QEvent>
#include <QFont>
#include <QGraphicsTextItem>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>

#include <cmath>
#include <limits>

namespace {
constexpr double kPriceEps = 0.0005;
constexpr double kMinFlatBodyHalfPrice = 0.0005;
/// 收平实体屏幕高度（像素）：细线，避免在普通 K 线中像实心柱
constexpr double kMinFlatBodyPixels = 2.0;
/// 有影线时实体最多占整根 K 线高度比例
constexpr double kMaxFlatBodyRangeRatio = 0.1;
constexpr QMargins kMainChartMargins{4, 4, 4, 0};
constexpr QMargins kSubChartMargins{0, 0, 0, 0};
const QColor kPanelWhite{0xFF, 0xFF, 0xFF};

struct DisplayOhlc
{
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
};

double flatBodyHeightForBar(const CandleBar &b, double flatBodyHalf)
{
    double height = flatBodyHalf * 2.0;
    const double range = b.high - b.low;
    if (range > kPriceEps) {
        height = qMin(height, range * kMaxFlatBodyRangeRatio);
    }
    return qMax(height, kPriceEps * 4);
}

DisplayOhlc ohlcForDisplay(const CandleBar &b, double flatBodyHalf)
{
    DisplayOhlc d{b.open, b.high, b.low, b.close};
    if (qAbs(d.close - d.open) > kPriceEps) {
        return d;
    }

    const double px = (d.open + d.close) * 0.5;
    const double bodyHeight = flatBodyHeightForBar(b, flatBodyHalf);
    const double half = bodyHeight * 0.5;

    const bool flatAtLow = b.low >= px - kPriceEps && qAbs(b.open - b.low) <= kPriceEps
        && qAbs(b.close - b.low) <= kPriceEps;
    const bool flatAtHigh = b.high <= px + kPriceEps && qAbs(b.open - b.high) <= kPriceEps
        && qAbs(b.close - b.high) <= kPriceEps;

    if (flatAtLow && !flatAtHigh) {
        d.low = b.low;
        d.open = b.low;
        d.close = b.low + bodyHeight;
        d.high = qMax(b.high, d.close);
        return d;
    }

    if (flatAtHigh && !flatAtLow) {
        d.high = b.high;
        d.close = b.high;
        d.open = b.high - bodyHeight;
        d.low = qMin(b.low, d.open);
        return d;
    }

    const double bodyBottom = px - half;
    const double bodyTop = px + half;
    d.open = bodyBottom;
    d.close = bodyTop;

    if (b.high <= bodyTop + kPriceEps && b.low >= bodyBottom - kPriceEps) {
        d.high = bodyTop;
        d.low = bodyBottom;
    } else {
        d.high = qMax(b.high, bodyTop);
        d.low = qMin(b.low, bodyBottom);
    }
    return d;
}

const QColor kEastMoneyUp{0xF0, 0x3E, 0x3E};
const QColor kEastMoneyDown{0x1B, 0xAA, 0x3A};
const QColor kLabelHighColor{0xF0, 0x3E, 0x3E};
const QColor kLabelLowColor{0x1B, 0xAA, 0x3A};
const QColor kSubChartTitleColor{0x66, 0x66, 0x66};
const QColor kMacdDifColor{0x44, 0x88, 0xFF};
const QColor kMacdDeaColor{0xFF, 0x88, 0x00};
const QColor kKdjKColor{0xFF, 0xC1, 0x07};
const QColor kKdjDColor{0x42, 0x85, 0xF4};
const QColor kKdjJColor{0xAB, 0x47, 0xBC};

QGraphicsTextItem *makeChartLabel(const QString &text, const QColor &color, QChart *chart,
                                bool bold = false, int pointSize = 9)
{
    auto *item = new QGraphicsTextItem(text, chart);
    item->setZValue(100);
    QFont font = item->font();
    font.setPointSize(pointSize);
    font.setBold(bold);
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
    series->setCapsVisible(false);
}

void styleLineSeries(QLineSeries *series, const QColor &color, int width = 1)
{
    QPen pen(color, width);
    series->setPen(pen);
    series->setBrush(Qt::NoBrush);
}

void clearBarSet(QBarSet *set)
{
    if (!set) {
        return;
    }
    const int n = set->count();
    if (n > 0) {
        set->remove(0, n);
    }
}

void hideAxis(QValueAxis *axis)
{
    axis->setTitleVisible(false);
    axis->setLabelsVisible(false);
    axis->setLineVisible(false);
    axis->setGridLineVisible(false);
    axis->setMinorGridLineVisible(false);
    axis->setShadesVisible(false);
}

void styleMainChartAxisY(QValueAxis *axis)
{
    axis->setTitleVisible(false);
    axis->setLabelsVisible(false);
    axis->setLineVisible(false);
    axis->setShadesVisible(false);
    axis->setMinorGridLineVisible(false);
    axis->setTickCount(7);

    QPen gridPen(QColor(0xE4, 0xE4, 0xE4));
    gridPen.setStyle(Qt::DashLine);
    gridPen.setWidthF(1.0);
    gridPen.setCosmetic(true);
    axis->setGridLinePen(gridPen);
    axis->setGridLineVisible(true);
}

void styleBackButton(QPushButton *button)
{
    button->setCursor(Qt::PointingHandCursor);
    button->setFlat(true);
    button->setText(QStringLiteral("←  返回"));
    button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  color: #333333;"
        "  background: #FFFFFF;"
        "  border: 1px solid #E0E0E0;"
        "  border-radius: 8px;"
        "  padding: 6px 14px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background: #F7F7F7;"
        "  border-color: #D0D0D0;"
        "}"
        "QPushButton:pressed {"
        "  background: #EEEEEE;"
        "}"));
}

void styleChartView(QChartView *view, const QMargins &chartMargins, bool plotAreaFill)
{
    auto *chart = view->chart();
    chart->setBackgroundRoundness(0);
    chart->setMargins(chartMargins);
    chart->setBackgroundBrush(QBrush(kPanelWhite));
    chart->setPlotAreaBackgroundVisible(plotAreaFill);
    if (plotAreaFill) {
        chart->setPlotAreaBackgroundBrush(QBrush(kPanelWhite));
    }

    view->setFrameShape(QFrame::NoFrame);
    view->setLineWidth(0);
    view->setAttribute(Qt::WA_OpaquePaintEvent, true);
    view->setAutoFillBackground(true);
    view->setBackgroundBrush(QBrush(kPanelWhite));
    view->setStyleSheet(QStringLiteral("QChartView{background:#FFFFFF;border:none;}"));
}

QFrame *makeChartDivider(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setFixedHeight(1);
    line->setStyleSheet(QStringLiteral("background-color:#E6E6E6;border:none;"));
    return line;
}

QString formatIndicatorValue(double value, bool valid, int decimals)
{
    if (!valid || !std::isfinite(value)) {
        return QStringLiteral("--");
    }
    return QString::number(value, 'f', decimals);
}

QString htmlColoredText(const QColor &color, const QString &text)
{
    return QStringLiteral("<span style=\"color:%1\">%2</span>")
        .arg(color.name(QColor::HexRgb), text.toHtmlEscaped());
}

QColor macdBarReadoutColor(const IndicatorBar &ind)
{
    if (!ind.macdBarValid || !std::isfinite(ind.macdBar)) {
        return kSubChartTitleColor;
    }
    return ind.macdBar >= 0.0 ? kEastMoneyUp : kEastMoneyDown;
}

QString formatMacdReadoutHtml(const IndicatorBar &ind)
{
    const QString dif =
        QStringLiteral("DIF:%1").arg(formatIndicatorValue(ind.macdDif, ind.macdDifValid, 3));
    const QString dea =
        QStringLiteral("DEA:%1").arg(formatIndicatorValue(ind.macdDea, ind.macdDeaValid, 3));
    const QString bar =
        QStringLiteral("MACD:%1").arg(formatIndicatorValue(ind.macdBar, ind.macdBarValid, 3));

    return htmlColoredText(kMacdDifColor, dif) + QStringLiteral("&nbsp;&nbsp;")
        + htmlColoredText(kMacdDeaColor, dea) + QStringLiteral("&nbsp;&nbsp;")
        + htmlColoredText(macdBarReadoutColor(ind), bar);
}

QString formatKdjReadoutHtml(const IndicatorBar &ind)
{
    const QString k =
        QStringLiteral("K:%1").arg(formatIndicatorValue(ind.kdjK, ind.kdjKValid, 2));
    const QString d =
        QStringLiteral("D:%1").arg(formatIndicatorValue(ind.kdjD, ind.kdjDValid, 2));
    const QString j =
        QStringLiteral("J:%1").arg(formatIndicatorValue(ind.kdjJ, ind.kdjJValid, 2));

    return htmlColoredText(kKdjKColor, k) + QStringLiteral("&nbsp;&nbsp;") + htmlColoredText(kKdjDColor, d)
        + QStringLiteral("&nbsp;&nbsp;") + htmlColoredText(kKdjJColor, j);
}

QString formatCandleDetail(const CandleBar &bar)
{
    const QTimeZone tz(QByteArrayLiteral("Asia/Shanghai"));
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(bar.tsSec, tz);
    const QString timeLine =
        dt.isValid() ? dt.toString(QStringLiteral("yyyy-MM-dd HH:mm")) : QStringLiteral("--");

    return QStringLiteral("%1\n开 %2  高 %3\n低 %4  收 %5\n量 %6")
        .arg(timeLine)
        .arg(bar.open, 0, 'f', 2)
        .arg(bar.high, 0, 'f', 2)
        .arg(bar.low, 0, 'f', 2)
        .arg(bar.close, 0, 'f', 2)
        .arg(static_cast<qint64>(bar.volume));
}
} // namespace

void StockDetailPage::setupSubChart(SubChart &panel, const QString &title)
{
    panel.chart = new QChart();
    panel.chart->legend()->hide();
    panel.chart->setAnimationOptions(QChart::NoAnimation);

    panel.title = makeChartLabel(title, kSubChartTitleColor, panel.chart, false, 10);
    panel.values = makeChartLabel(QString(), kSubChartTitleColor, panel.chart, false, 9);

    panel.axisX = new QValueAxis();
    hideAxis(panel.axisX);

    panel.axisY = new QValueAxis();
    hideAxis(panel.axisY);

    panel.chart->addAxis(panel.axisX, Qt::AlignBottom);
    panel.chart->addAxis(panel.axisY, Qt::AlignLeft);

    panel.view = new QChartView(panel.chart);
    // 副图关闭 plotArea 独立底色，避免上下留白呈「方框」
    styleChartView(panel.view, kSubChartMargins, false);
    panel.view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    panel.view->setRenderHint(QPainter::Antialiasing, false);
    panel.view->setDragMode(QGraphicsView::NoDrag);
}

StockDetailPage::StockDetailPage(QWidget *parent)
    : QWidget(parent)
    , m_title(new QLabel(this))
    , m_back(new QPushButton(this))
    , m_timeline(new KlineTimelineBar(this))
    , m_chart(new QChart())
    , m_view(new QChartView(m_chart))
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_seriesUp(new QCandlestickSeries())
    , m_seriesDown(new QCandlestickSeries())
    , m_macdHist(new QBarSeries())
    , m_macdDif(new QLineSeries())
    , m_macdDea(new QLineSeries())
    , m_kdjK(new QLineSeries())
    , m_kdjD(new QLineSeries())
    , m_kdjJ(new QLineSeries())
    , m_visibleBarCount(KlineLoadConfig::VisibleBarCount)
{
    m_chart->legend()->hide();
    m_chart->setAnimationOptions(QChart::NoAnimation);
    m_chart->setMargins(kMainChartMargins);

    styleCandleSeries(m_seriesUp, kEastMoneyUp);
    styleCandleSeries(m_seriesDown, kEastMoneyDown);

    m_chart->addSeries(m_seriesUp);
    m_chart->addSeries(m_seriesDown);

    m_axisX->setTitleVisible(false);
    m_axisX->setLabelsVisible(false);
    m_axisX->setGridLineVisible(false);
    m_axisX->setLineVisible(false);

    styleMainChartAxisY(m_axisY);

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    for (QCandlestickSeries *s : {m_seriesUp, m_seriesDown}) {
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisY);
    }

    m_labelHigh = makeChartLabel(QString(), kLabelHighColor, m_chart, true);
    m_labelLow = makeChartLabel(QString(), kLabelLowColor, m_chart, true);
    m_labelHigh->hide();
    m_labelLow->hide();

    m_candleDetail = makeChartLabel(QString(), QColor(0x33, 0x33, 0x33), m_chart, false, 9);
    m_candleDetail->setZValue(120);
    m_candleDetail->hide();

    styleChartView(m_view, kMainChartMargins, false);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_view->setRenderHint(QPainter::Antialiasing, false);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->viewport()->setAutoFillBackground(true);
    m_view->viewport()->setStyleSheet(QStringLiteral("background:#FFFFFF;border:none;"));
    m_view->viewport()->installEventFilter(this);

    setupSubChart(m_macdChart, QStringLiteral("MACD"));
    m_macdHist->setBarWidth(0.72);
    m_macdHistUp = new QBarSet(QStringLiteral("up"));
    m_macdHistUp->setColor(kEastMoneyUp);
    m_macdHistDown = new QBarSet(QStringLiteral("down"));
    m_macdHistDown->setColor(kEastMoneyDown);
    m_macdHist->append(m_macdHistUp);
    m_macdHist->append(m_macdHistDown);
    styleLineSeries(m_macdDif, kMacdDifColor, 1);
    styleLineSeries(m_macdDea, kMacdDeaColor, 1);
    m_macdChart.chart->addSeries(m_macdHist);
    m_macdChart.chart->addSeries(m_macdDif);
    m_macdChart.chart->addSeries(m_macdDea);
    m_macdHist->attachAxis(m_macdChart.axisX);
    m_macdHist->attachAxis(m_macdChart.axisY);
    for (QLineSeries *s : {m_macdDif, m_macdDea}) {
        s->attachAxis(m_macdChart.axisX);
        s->attachAxis(m_macdChart.axisY);
    }

    setupSubChart(m_kdjChart, QStringLiteral("KDJ"));
    styleLineSeries(m_kdjK, kKdjKColor, 1);
    styleLineSeries(m_kdjD, kKdjDColor, 1);
    styleLineSeries(m_kdjJ, kKdjJColor, 1);
    m_kdjChart.chart->addSeries(m_kdjK);
    m_kdjChart.chart->addSeries(m_kdjD);
    m_kdjChart.chart->addSeries(m_kdjJ);
    for (QLineSeries *s : {m_kdjK, m_kdjD, m_kdjJ}) {
        s->attachAxis(m_kdjChart.axisX);
        s->attachAxis(m_kdjChart.axisY);
    }

    styleBackButton(m_back);

    auto *periodLabel = new QLabel(tr("5分钟"), this);
    periodLabel->setStyleSheet(
        QStringLiteral("color:#888;font-size:12px;padding:6px 10px;"
                         "background:#FFF;border:1px solid #E8E8E8;border-radius:8px;"));

    auto *top = new QHBoxLayout();
    top->setSpacing(12);
    top->addWidget(m_back, 0, Qt::AlignLeft);
    top->addStretch(1);
    top->addWidget(periodLabel, 0, Qt::AlignRight);

    m_title->setStyleSheet(QStringLiteral("color:#222;font-size:15px;font-weight:600;padding:0;"));

    auto *chartPanel = new QFrame(this);
    chartPanel->setObjectName(QStringLiteral("chartPanel"));
    chartPanel->setAttribute(Qt::WA_StyledBackground, true);
    chartPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartPanel->setStyleSheet(QStringLiteral(
        "#chartPanel {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #DCDCDC;"
        "  border-radius: 12px;"
        "}"));

    auto *panelLayout = new QVBoxLayout(chartPanel);
    panelLayout->setContentsMargins(8, 8, 8, 10);
    panelLayout->setSpacing(0);
    panelLayout->addWidget(m_view, 4);
    panelLayout->addWidget(makeChartDivider(chartPanel));
    panelLayout->addWidget(m_macdChart.view, 2);
    panelLayout->addWidget(makeChartDivider(chartPanel));
    panelLayout->addWidget(m_kdjChart.view, 2);
    panelLayout->addWidget(m_timeline, 0);

    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral("StockDetailPage { background-color: #F3F4F6; }"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);
    root->addLayout(top);
    root->addWidget(m_title);
    root->addWidget(chartPanel, 1);

    connect(m_back, &QPushButton::clicked, this, &StockDetailPage::backRequested);
    connect(m_timeline, &KlineTimelineBar::valueChanged, this, [this](int) {
        if (m_updatingScroll || m_candles.isEmpty()) {
            return;
        }
        renderVisibleWindow();
        if (!m_timeline->isDragging()) {
            checkPrefetch();
        }
    });
    connect(m_timeline, &KlineTimelineBar::interactionEnded, this, [this]() {
        if (!m_candles.isEmpty()) {
            checkPrefetch();
        }
    });
    connect(m_chart, &QChart::plotAreaChanged, this, &StockDetailPage::updateExtremaLabels);
    connect(m_macdChart.chart, &QChart::plotAreaChanged, this, &StockDetailPage::updateSubChartHeaders);
    connect(m_kdjChart.chart, &QChart::plotAreaChanged, this, &StockDetailPage::updateSubChartHeaders);
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
    m_indicators.clear();
    m_oldestLoadedIndex = 0;
    m_totalCount = 0;
    m_prefetchInFlight = false;
    m_extremaLocalHighIdx = -1;
    m_extremaLocalLowIdx = -1;
    m_seriesUp->clear();
    m_seriesDown->clear();
    clearBarSet(m_macdHistUp);
    clearBarSet(m_macdHistDown);
    m_macdDif->clear();
    m_macdDea->clear();
    m_kdjK->clear();
    m_kdjD->clear();
    m_kdjJ->clear();
    m_labelHigh->hide();
    m_labelLow->hide();
    hideCandleDetail();
    m_focusBarIndex = -1;
    m_updatingScroll = true;
    m_timeline->setCandles({}, m_visibleBarCount);
    m_timeline->setRange(0, 0);
    m_timeline->setValue(0);
    m_updatingScroll = false;
}

void StockDetailPage::mergeCandles(const QString &symbol, quint64 startIndex, quint64 total,
                                   const QVector<CandleBar> &candles,
                                   const QVector<IndicatorBar> &indicators)
{
    if (symbol != m_symbol || candles.isEmpty() || indicators.size() != candles.size()) {
        return;
    }

    m_prefetchInFlight = false;
    m_totalCount = total;

    const bool firstLoad = m_candles.isEmpty();
    const int scrollBefore = m_timeline->value();

    if (firstLoad) {
        m_candles = candles;
        m_indicators = indicators;
        m_oldestLoadedIndex = startIndex;
    } else if (startIndex < m_oldestLoadedIndex) {
        const int added = static_cast<int>(candles.size());
        QVector<CandleBar> mergedCandles;
        mergedCandles.reserve(added + m_candles.size());
        mergedCandles += candles;
        mergedCandles += m_candles;

        QVector<IndicatorBar> mergedIndicators;
        mergedIndicators.reserve(added + m_indicators.size());
        mergedIndicators += indicators;
        mergedIndicators += m_indicators;

        m_candles = std::move(mergedCandles);
        m_indicators = std::move(mergedIndicators);
        m_oldestLoadedIndex = startIndex;

        syncTimelineRange();
        m_updatingScroll = true;
        m_timeline->setValue(scrollBefore + added);
        m_updatingScroll = false;

        renderVisibleWindow();
        return;
    } else {
        return;
    }

    syncTimelineRange();
    scrollToLatest();
    renderVisibleWindow();
}

void StockDetailPage::updateFlatBodyHalfForAxis(double axisMin, double axisMax)
{
    const double span = axisMax - axisMin;
    if (!(span > 0.0) || !std::isfinite(span)) {
        m_flatBodyHalf = kMinFlatBodyHalfPrice;
        return;
    }

    const double plotHeight = qMax(80.0, m_view->height() * 0.72);

    const double pricePerPixel = span / plotHeight;
    m_flatBodyHalf = qMax(kMinFlatBodyHalfPrice, pricePerPixel * kMinFlatBodyPixels * 0.5);
}

void StockDetailPage::appendCandle(const CandleBar &b, int localIndex)
{
    const DisplayOhlc d = ohlcForDisplay(b, m_flatBodyHalf);
    auto *set = new QCandlestickSet(d.open, d.high, d.low, d.close, static_cast<qreal>(localIndex));
    if (b.close >= b.open - kPriceEps) {
        m_seriesUp->append(set);
    } else {
        m_seriesDown->append(set);
    }
}

void StockDetailPage::fitAxisY(QValueAxis *axis, const QVector<double> &samples)
{
    double minV = std::numeric_limits<double>::infinity();
    double maxV = -std::numeric_limits<double>::infinity();
    for (double v : samples) {
        if (!std::isfinite(v)) {
            continue;
        }
        minV = qMin(minV, v);
        maxV = qMax(maxV, v);
    }
    if (!std::isfinite(minV) || !std::isfinite(maxV) || minV >= maxV) {
        axis->setRange(-1, 1);
        return;
    }
    const double pad = (maxV - minV) * 0.08;
    axis->setRange(minV - pad, maxV + pad);
}

void StockDetailPage::syncSubChartXRange(int count)
{
    const qreal xMax = static_cast<qreal>(count) - 0.5;
    m_axisX->setRange(-0.5, xMax);
    m_macdChart.axisX->setRange(-0.5, xMax);
    m_kdjChart.axisX->setRange(-0.5, xMax);
}

void StockDetailPage::updateSubChartHeaders()
{
    const auto layoutRow = [](SubChart &panel) {
        if (!panel.title || !panel.values || !panel.chart) {
            return;
        }
        const QRectF plot = panel.chart->plotArea();
        const qreal top = plot.top() + 2;
        panel.title->setPos(plot.left() + 4, top);
        const qreal valuesX = panel.title->pos().x() + panel.title->boundingRect().width() + 8;
        panel.values->setPos(valuesX, top + 1);
    };
    layoutRow(m_macdChart);
    layoutRow(m_kdjChart);
}

void StockDetailPage::updateIndicatorReadouts(int barIndex)
{
    if (barIndex < 0 || barIndex >= m_indicators.size()) {
        if (m_macdChart.values) {
            m_macdChart.values->setHtml(QString());
        }
        if (m_kdjChart.values) {
            m_kdjChart.values->setHtml(QString());
        }
        updateSubChartHeaders();
        return;
    }

    const IndicatorBar &ind = m_indicators[barIndex];
    m_macdChart.values->setHtml(formatMacdReadoutHtml(ind));
    m_kdjChart.values->setHtml(formatKdjReadoutHtml(ind));
    updateSubChartHeaders();
}

int StockDetailPage::defaultBarIndex() const
{
    if (m_candles.isEmpty()) {
        return -1;
    }

    const QTimeZone tz(QByteArrayLiteral("Asia/Shanghai"));
    const QDate latestDay =
        QDateTime::fromSecsSinceEpoch(m_candles.last().tsSec, tz).date();

    for (int i = m_candles.size() - 1; i >= 0; --i) {
        const QDate day = QDateTime::fromSecsSinceEpoch(m_candles[i].tsSec, tz).date();
        if (day == latestDay) {
            return i;
        }
        if (day < latestDay) {
            break;
        }
    }
    return m_candles.size() - 1;
}

void StockDetailPage::showCandleDetailAt(int barIndex, const QPoint &viewPos)
{
    if (barIndex < 0 || barIndex >= m_candles.size() || !m_candleDetail) {
        return;
    }

    m_candleDetail->setPlainText(formatCandleDetail(m_candles[barIndex]));

    const QPointF scenePos = m_view->mapToScene(viewPos);
    QPointF chartPos = m_chart->mapFromScene(scenePos);
    const QRectF plot = m_chart->plotArea();
    const QRectF bounds = m_candleDetail->boundingRect();

    qreal x = chartPos.x() + 10;
    qreal y = chartPos.y() + 10;
    x = qBound(plot.left(), x, plot.right() - bounds.width());
    y = qBound(plot.top(), y, plot.bottom() - bounds.height());
    m_candleDetail->setPos(x, y);
    m_candleDetail->show();
}

void StockDetailPage::hideCandleDetail()
{
    if (m_candleDetail) {
        m_candleDetail->hide();
    }
}

bool StockDetailPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_view->viewport()) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton && m_candleDetail && m_candleDetail->isVisible()) {
            hideCandleDetail();
            m_focusBarIndex = -1;
            updateIndicatorReadouts(defaultBarIndex());
        }
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() != Qt::LeftButton || m_candles.isEmpty()) {
            return QWidget::eventFilter(watched, event);
        }

        const QPointF scenePos = m_view->mapToScene(mouseEvent->pos());
        const QPointF valuePos = m_chart->mapToValue(scenePos, m_seriesUp);
        const int start = m_timeline->value();
        const int end = qMin(start + m_visibleBarCount, m_candles.size());
        const int localIdx = qBound(0, qRound(valuePos.x()), qMax(0, end - start - 1));
        const int barIndex = start + localIdx;

        m_focusBarIndex = barIndex;
        showCandleDetailAt(barIndex, mouseEvent->pos());
        updateIndicatorReadouts(barIndex);
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void StockDetailPage::renderMacdChart(int start, int end)
{
    clearBarSet(m_macdHistUp);
    clearBarSet(m_macdHistDown);
    m_macdDif->clear();
    m_macdDea->clear();

    QVector<double> samples;
    samples.reserve((end - start) * 3);

    for (int i = start; i < end; ++i) {
        const IndicatorBar &ind = m_indicators[i];
        const qreal x = static_cast<qreal>(i - start);

        if (ind.macdBarValid) {
            const double bar = ind.macdBar;
            if (bar >= 0.0) {
                m_macdHistUp->append(bar);
                m_macdHistDown->append(0.0);
            } else {
                m_macdHistUp->append(0.0);
                m_macdHistDown->append(bar);
            }
            samples.append(bar);
        } else {
            m_macdHistUp->append(0.0);
            m_macdHistDown->append(0.0);
        }

        if (ind.macdDifValid) {
            m_macdDif->append(x, ind.macdDif);
            samples.append(ind.macdDif);
        }
        if (ind.macdDeaValid) {
            m_macdDea->append(x, ind.macdDea);
            samples.append(ind.macdDea);
        }
    }

    samples.append(0.0);
    fitAxisY(m_macdChart.axisY, samples);
}

void StockDetailPage::renderKdjChart(int start, int end)
{
    m_kdjK->clear();
    m_kdjD->clear();
    m_kdjJ->clear();

    QVector<double> samples;
    samples.reserve((end - start) * 3);

    for (int i = start; i < end; ++i) {
        const IndicatorBar &ind = m_indicators[i];
        const qreal x = static_cast<qreal>(i - start);
        if (ind.kdjKValid) {
            m_kdjK->append(x, ind.kdjK);
            samples.append(ind.kdjK);
        }
        if (ind.kdjDValid) {
            m_kdjD->append(x, ind.kdjD);
            samples.append(ind.kdjD);
        }
        if (ind.kdjJValid) {
            m_kdjJ->append(x, ind.kdjJ);
            samples.append(ind.kdjJ);
        }
    }

    if (samples.isEmpty()) {
        m_kdjChart.axisY->setRange(0, 100);
    } else {
        fitAxisY(m_kdjChart.axisY, samples);
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
        clearBarSet(m_macdHistUp);
        clearBarSet(m_macdHistDown);
        m_macdDif->clear();
        m_macdDea->clear();
        m_kdjK->clear();
        m_kdjD->clear();
        m_kdjJ->clear();
        return;
    }

    const int start = m_timeline->value();
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
        minY = qMin(minY, b.low);
        maxY = qMax(maxY, b.high);
    }

    double axisMin = minY;
    double axisMax = maxY;
    if (std::isfinite(minY) && std::isfinite(maxY) && minY < maxY) {
        const double pad = (maxY - minY) * 0.05;
        axisMin = minY - pad;
        axisMax = maxY + pad;
        m_axisY->setRange(axisMin, axisMax);
    }
    updateFlatBodyHalfForAxis(axisMin, axisMax);

    for (int i = start; i < end; ++i) {
        const CandleBar &b = m_candles[i];
        const int localIdx = i - start;
        appendCandle(b, localIdx);

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

    syncSubChartXRange(count);

    renderMacdChart(start, end);
    renderKdjChart(start, end);

    const int readoutIndex =
        m_focusBarIndex >= 0 ? m_focusBarIndex : defaultBarIndex();

    QTimer::singleShot(0, this, [this, readoutIndex]() {
        updateExtremaLabels();
        updateIndicatorReadouts(readoutIndex);
    });
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

void StockDetailPage::syncTimelineRange()
{
    const int n = m_candles.size();
    const int maxStart = qMax(0, n - m_visibleBarCount);

    m_timeline->setCandles(m_candles, m_visibleBarCount);

    m_updatingScroll = true;
    m_timeline->setRange(0, maxStart);
    if (m_timeline->value() > maxStart) {
        m_timeline->setValue(maxStart);
    }
    m_updatingScroll = false;
}

void StockDetailPage::scrollToLatest()
{
    const int maxStart = qMax(0, m_candles.size() - m_visibleBarCount);
    m_updatingScroll = true;
    m_timeline->setValue(maxStart);
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

    const int start = m_timeline->value();
    const qint64 oldestTs = m_candles.first().tsSec;
    const qint64 loadedSpan = qMax<qint64>(1, m_candles.last().tsSec - oldestTs);

    const qint64 bufferSec = loadedSpan * KlineLoadConfig::PrefetchBufferTradingDays
        / qMax(1, KlineLoadConfig::InitialTradingDays);

    const qint64 viewportOldestTs = m_candles[qBound(0, start, m_candles.size() - 1)].tsSec;
    if (viewportOldestTs - oldestTs <= bufferSec) {
        m_prefetchInFlight = true;
        emit needOlderCandles(m_symbol, m_oldestLoadedIndex);
    }
}
