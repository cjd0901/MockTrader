#pragma once

#include "net/KlineBinary.h"

#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QCandlestickSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QVector>
#include <QWidget>

class QLabel;
class QGraphicsTextItem;
class QPushButton;
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

signals:
    void backRequested();
    void needOlderCandles(const QString &symbol, quint64 beforeIndex);

private:
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
    void syncTimelineRange();
    void scrollToLatest();
    void checkPrefetch();
    void placeLabelBesideCandle(QGraphicsTextItem *label, const QPointF &anchor, bool preferLeft);
    void appendCandle(const CandleBar &b, int localIndex);
    void updateFlatBodyHalfForAxis(double axisMin, double axisMax);
    static void setupSubChart(SubChart &panel, const QString &title);
    static void fitAxisY(QValueAxis *axis, const QVector<double> &samples);

    QString m_symbol;
    QString m_displayName;

    QLabel *m_title = nullptr;
    QPushButton *m_back = nullptr;
    KlineTimelineBar *m_timeline = nullptr;

    QChart *m_chart = nullptr;
    QChartView *m_view = nullptr;
    QValueAxis *m_axisX = nullptr;
    QValueAxis *m_axisY = nullptr;
    QCandlestickSeries *m_seriesUp = nullptr;
    QCandlestickSeries *m_seriesDown = nullptr;

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
