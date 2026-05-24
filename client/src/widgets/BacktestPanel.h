#pragma once

#include "model/MarketTypes.h"

#include <QWidget>

class QFrame;
class QLineEdit;
class QStackedWidget;
class StrategyPicker;
class QLabel;
class QPushButton;

class BacktestPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit BacktestPanel(QWidget *parent = nullptr);

    /// Full range from server file (resets pickers to file min/max).
    void setStrategies(const QVector<StrategyRow> &strategies);
    void setFileTimeRange(qint64 minTsSec, qint64 maxTsSec);
    /// Widen selectable range when more K-line history is loaded (keeps user selection).
    void expandLoadedRange(qint64 minTsSec, qint64 maxTsSec);
    void setRunning(bool running);
    void setBacktestResult(const BacktestSummary &summary, int buyCount, int sellCount);
    void setResultText(const QString &text);

signals:
    void runRequested(const QString &strategyId, qint64 startTs, qint64 endTs);

private:
    void onRunClicked();
    void onTimeEditingFinished();
    void updateRangeLabel();
    void applyDateTimeLimits();
    void clampTimeInputs();
    void setTimeEditText(QLineEdit *edit, qint64 tsSec);
    qint64 parseTimeEdit(const QLineEdit *edit, bool *ok) const;
    void showPnlCard(bool show);
    void setPnlPlaceholder(const QString &text);

    QLabel *makeFieldLabel(const QString &text, QWidget *parent);
    QLabel *makeMetricValue(const QString &objectName, QWidget *parent);
    QWidget *makeMetricRow(const QString &caption, QLabel *valueLabel, QWidget *parent);

    StrategyPicker *m_strategy = nullptr;
    QLineEdit *m_startTime = nullptr;
    QLineEdit *m_endTime = nullptr;
    QLabel *m_rangeHint = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_runButton = nullptr;

    QFrame *m_pnlCard = nullptr;
    QStackedWidget *m_pnlStack = nullptr;
    QWidget *m_metricsContainer = nullptr;
    QLabel *m_pnlPlaceholder = nullptr;
    QLabel *m_returnValue = nullptr;
    QLabel *m_initialCapitalValue = nullptr;
    QLabel *m_finalEquityValue = nullptr;
    QLabel *m_tradesValue = nullptr;
    QLabel *m_openPositionNote = nullptr;
    QLabel *m_signalSummary = nullptr;

    qint64 m_minTs = 0;
    qint64 m_maxTs = 0;
    qint64 m_fileMinTs = 0;
    qint64 m_fileMaxTs = 0;
};
