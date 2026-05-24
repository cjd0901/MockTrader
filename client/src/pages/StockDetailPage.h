#pragma once

#include "model/MarketTypes.h"

#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QCandlestickSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QVector>
#include <QWidget>

class QLabel;
class QGraphicsLineItem;
class QGraphicsTextItem;
class QPushButton;
class BacktestPanel;
class KlineTimelineBar;

class StockDetailPage final : public QWidget
{
    Q_OBJECT

public:
    explicit StockDetailPage(QWidget *parent = nullptr);

    void setStock(const QString &symbol, const QString &displayName);
    void resetCandles();

public slots:
    void mergeCandles(const QString &symbol, quint64 startIndex, quint64 total,
                      const QVector<CandleBar> &candles, const QVector<IndicatorBar> &indicators);
    void setBacktestSignals(const QVector<TradeSignal> &tradeSignals, const BacktestSummary &summary);
    void setBacktestRunning(bool running);
    void setBacktestError(const QString &message);
    void setKlineFileTimeRange(const QString &symbol, qint64 minTsSec, qint64 maxTsSec);
    void setBacktestStrategies(const QVector<StrategyRow> &strategies);

signals:
    void backRequested();
    void needOlderCandles(const QString &symbol, quint64 beforeIndex);
    void backtestRequested(const QString &symbol, const QString &strategyId, qint64 startTs,
                           qint64 endTs);

private:
    struct ChartTradeMarker {
        QGraphicsLineItem *stem = nullptr;
        QGraphicsTextItem *label = nullptr;
    };

    struct SubChart {
        QGraphicsTextItem *title = nullptr;
        QGraphicsTextItem *values = nullptr;
        QChart *chart = nullptr;
        QChartView *view = nullptr;
        QValueAxis *axisX = nullptr;
        QValueAxis *axisY = nullptr;
    };

    void renderVisibleWindow();
    void renderMacdChart(int start, int end);
    void renderKdjChart(int start, int end);
    void syncSubChartXRange(int count);
    void updateExtremaLabels();
    void updateSubChartHeaders();
    void updateIndicatorReadouts(int barIndex);
    int defaultBarIndex() const;
    void showCandleDetailAt(int barIndex, const QPoint &viewPos);
    void hideCandleDetail();
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void syncTimelineRange();
    void scrollToLatest();
    void checkPrefetch();
    void placeLabelBesideCandle(QGraphicsTextItem *label, const QPointF &anchor, bool preferLeft);
    void appendCandle(const CandleBar &b, int localIndex);
    void updateFlatBodyHalfForAxis(double axisMin, double axisMax);
    void updateBacktestTimeBounds();
    void clearTradeMarkers();
    void clearBacktestRangeBands();
    void updateBacktestRangeBands(int visibleStart, int visibleCount);
    void updateBacktestMarkers(int visibleStart, int visibleCount);
    void clearBacktestOverlay();
    void positionClearBacktestButton();
    static void setupSubChart(SubChart &panel, const QString &title);
    static void fitAxisY(QValueAxis *axis, const QVector<double> &samples);

    QString m_symbol;
    QString m_displayName;

    QLabel *m_title = nullptr;
    QPushButton *m_back = nullptr;
    QPushButton *m_clearBacktestBtn = nullptr;
    QFrame *m_chartPanel = nullptr;
    KlineTimelineBar *m_timeline = nullptr;

    QChart *m_chart = nullptr;
    QChartView *m_view = nullptr;
    QValueAxis *m_axisX = nullptr;
    QValueAxis *m_axisY = nullptr;
    QLineSeries *m_backtestAreaUpper = nullptr;
    QLineSeries *m_backtestAreaLower = nullptr;
    QAreaSeries *m_backtestArea = nullptr;
    QCandlestickSeries *m_seriesUp = nullptr;
    QCandlestickSeries *m_seriesDown = nullptr;
    BacktestPanel *m_backtestPanel = nullptr;
    QVector<ChartTradeMarker> m_tradeMarkers;
    qint64 m_backtestRangeStartTs = 0;
    qint64 m_backtestRangeEndTs = 0;
    bool m_backtestOverlayActive = false;

    SubChart m_macdChart;
    QBarSeries *m_macdHist = nullptr;
    QBarSet *m_macdHistUp = nullptr;
    QBarSet *m_macdHistDown = nullptr;
    QLineSeries *m_macdDif = nullptr;
    QLineSeries *m_macdDea = nullptr;

    SubChart m_kdjChart;
    QLineSeries *m_kdjK = nullptr;
    QLineSeries *m_kdjD = nullptr;
    QLineSeries *m_kdjJ = nullptr;

    QGraphicsTextItem *m_labelHigh = nullptr;
    QGraphicsTextItem *m_labelLow = nullptr;
    QGraphicsTextItem *m_candleDetail = nullptr;

    int m_focusBarIndex = -1;

    QVector<CandleBar> m_candles;
    QVector<IndicatorBar> m_indicators;
    QVector<TradeSignal> m_backtestSignals;
    quint64 m_oldestLoadedIndex = 0;
    quint64 m_totalCount = 0;
    bool m_prefetchInFlight = false;

    int m_visibleBarCount = 0;
    bool m_updatingScroll = false;

    int m_extremaLocalHighIdx = -1;
    int m_extremaLocalLowIdx = -1;
    double m_extremaHigh = 0.0;
    double m_extremaLow = 0.0;

    /// 收平 K 线绘制用实体半高（价），同屏统一，由像素最小高度换算
    double m_flatBodyHalf = 0.001;
};
