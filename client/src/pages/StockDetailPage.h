#pragma once

#include "net/KlineBinary.h"

#include <QtCharts/QCandlestickSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>

#include <QVector>
#include <QWidget>

class QLabel;
class QGraphicsTextItem;
class QPushButton;
class QScrollBar;

class StockDetailPage final : public QWidget
{
    Q_OBJECT

public:
    explicit StockDetailPage(QWidget *parent = nullptr);

    void setStock(const QString &symbol, const QString &displayName);
    void resetCandles();

public slots:
    void mergeCandles(const QString &symbol, quint64 startIndex, quint64 total,
                      const QVector<CandleBar> &candles);

signals:
    void backRequested();
    void needOlderCandles(const QString &symbol, quint64 beforeIndex);

private:
    void renderVisibleWindow();
    void updateExtremaLabels();
    void syncScrollBarRange();
    void scrollToLatest();
    void checkPrefetch();
    void placeLabelBesideCandle(QGraphicsTextItem *label, const QPointF &anchor, bool preferLeft);
    void appendCandle(const CandleBar &b, int localIndex);

    QString m_symbol;
    QString m_displayName;

    QLabel *m_title = nullptr;
    QPushButton *m_back = nullptr;
    QScrollBar *m_scrollBar = nullptr;

    QChart *m_chart = nullptr;
    QChartView *m_view = nullptr;
    QValueAxis *m_axisX = nullptr;
    QValueAxis *m_axisY = nullptr;
    QCandlestickSeries *m_seriesUp = nullptr;
    QCandlestickSeries *m_seriesDown = nullptr;

    QGraphicsTextItem *m_labelHigh = nullptr;
    QGraphicsTextItem *m_labelLow = nullptr;

    QVector<CandleBar> m_candles;
    quint64 m_oldestLoadedIndex = 0;
    quint64 m_totalCount = 0;
    bool m_prefetchInFlight = false;

    int m_visibleBarCount = 0;
    bool m_updatingScroll = false;

    int m_extremaLocalHighIdx = -1;
    int m_extremaLocalLowIdx = -1;
    double m_extremaHigh = 0.0;
    double m_extremaLow = 0.0;
};
