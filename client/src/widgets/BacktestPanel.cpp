#include "widgets/BacktestPanel.h"

#include "widgets/StrategyPicker.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QTimeZone>
#include <QVBoxLayout>

namespace {

const QTimeZone kTz(QByteArrayLiteral("Asia/Shanghai"));
const QString kTimeFormat = QStringLiteral("yyyy-MM-dd HH:mm");
constexpr int kPanelWidth = 320;

QString formatTs(qint64 tsSec)
{
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(tsSec, kTz);
    return dt.isValid() ? dt.toString(kTimeFormat) : QStringLiteral("--");
}

QString formatMoney(const QLocale &locale, double value)
{
    return locale.toString(value, 'f', 2) + QStringLiteral(" 元");
}

QString formatReturnPct(const QLocale &locale, double pct)
{
    const QString num = locale.toString(pct, 'f', 2);
    return pct >= 0.0 ? QStringLiteral("+%1%").arg(num) : QStringLiteral("%1%").arg(num);
}

QString panelStyleSheet()
{
    return QStringLiteral(
        "#backtestPanel {"
        "  background:#FFFFFF;"
        "  border:1px solid #DCDCDC;"
        "  border-radius:12px;"
        "}"
        "#backtestPanel QLabel { color:#333; }"
        "#backtestPanel QLabel#backtestFieldLabel {"
        "  color:#6B7280;"
        "  font-size:12px;"
        "}"
        "#backtestPanel QFrame#backtestStrategyPicker,"
        "#backtestPanel QLineEdit#backtestTimeInput {"
        "  min-height:36px;"
        "  max-height:36px;"
        "  border:1px solid #D1D5DB;"
        "  border-radius:12px;"
        "  background:#FFFFFF;"
        "  outline:none;"
        "}"
        "#backtestPanel QFrame#backtestStrategyPicker:hover,"
        "#backtestPanel QLineEdit#backtestTimeInput:hover {"
        "  border-color:#9CA3AF;"
        "}"
        "#backtestPanel QFrame#backtestStrategyPicker:focus,"
        "#backtestPanel QLineEdit#backtestTimeInput:focus {"
        "  border-color:#4285F4;"
        "}"
        "#backtestPanel QFrame#backtestStrategyPicker[open=\"true\"] {"
        "  border-color:#4285F4;"
        "}"
        "#backtestPanel QLabel#strategyPickerLabel {"
        "  color:#222;"
        "  font-size:13px;"
        "  background:transparent;"
        "  border:none;"
        "  padding:0;"
        "  margin:0;"
        "}"
        "#backtestPanel QLabel#strategyPickerChevron {"
        "  color:#6B7280;"
        "  font-size:13px;"
        "  background:transparent;"
        "  border:none;"
        "  padding:0;"
        "  margin:0;"
        "}"
        "QMenu#backtestStrategyMenu {"
        "  border:1px solid #D1D5DB;"
        "  border-radius:0;"
        "  background:#FFFFFF;"
        "  padding:4px 0;"
        "  outline:none;"
        "}"
        "QMenu#backtestStrategyMenu::item {"
        "  padding:8px 12px;"
        "  color:#222;"
        "  font-size:13px;"
        "  border:none;"
        "}"
        "QMenu#backtestStrategyMenu::item:selected {"
        "  background:#EFF6FF;"
        "  color:#1D4ED8;"
        "}"
        "#backtestPanel QLineEdit#backtestTimeInput {"
        "  padding:0 10px;"
        "  color:#222;"
        "  font-size:13px;"
        "  font-family:Menlo,Consolas,monospace;"
        "}"
        "#backtestPanel QLineEdit#backtestTimeInput[invalid=\"true\"] {"
        "  border-color:#F03E3E;"
        "  background:#FFF5F5;"
        "}"
        "#backtestPanel QPushButton {"
        "  background:#4285F4;"
        "  color:#FFF;"
        "  border:none;"
        "  border-radius:8px;"
        "  padding:10px;"
        "  font-weight:600;"
        "}"
        "#backtestPanel QPushButton:disabled { background:#B0C4F8; }"
        "#backtestPanel QPushButton:hover:!disabled { background:#3367D6; }"
        "#backtestPanel QLabel#backtestStatusLabel {"
        "  color:#6B7280;"
        "  font-size:12px;"
        "}"
        "#backtestPanel QLabel#backtestStatusLabel[error=\"true\"] {"
        "  color:#F03E3E;"
        "}"
        "#backtestPanel QFrame#backtestPnlCard {"
        "  background:#F8FAFC;"
        "  border:1px solid #E5E7EB;"
        "  border-radius:8px;"
        "}"
        "#backtestPanel QFrame#backtestPnlCard QLabel,"
        "#backtestPanel QFrame#backtestPnlCard QWidget#backtestPnlBody,"
        "#backtestPanel QFrame#backtestPnlCard QWidget#backtestMetricRow,"
        "#backtestPanel QFrame#backtestPnlCard QStackedWidget {"
        "  background:transparent;"
        "  border:none;"
        "}"
        "#backtestPanel QLabel#backtestPnlSectionTitle {"
        "  color:#374151;"
        "  font-size:13px;"
        "  font-weight:600;"
        "}"
        "#backtestPanel QLabel#backtestReturnCaption {"
        "  color:#6B7280;"
        "  font-size:12px;"
        "}"
        "#backtestPanel QLabel#backtestReturnValue {"
        "  font-size:30px;"
        "  font-weight:700;"
        "  padding:2px 0 6px 0;"
        "}"
        "#backtestPanel QLabel#backtestReturnValue[positive=\"true\"] {"
        "  color:#E03131;"
        "}"
        "#backtestPanel QLabel#backtestReturnValue[positive=\"false\"] {"
        "  color:#2B8A3E;"
        "}"
        "#backtestPanel QLabel#backtestReturnValue[neutral=\"true\"] {"
        "  color:#6B7280;"
        "}"
        "#backtestPanel QLabel#backtestMetricCaption {"
        "  color:#6B7280;"
        "  font-size:12px;"
        "}"
        "#backtestPanel QLabel#backtestMetricValue {"
        "  color:#111827;"
        "  font-size:13px;"
        "  font-weight:600;"
        "}"
        "#backtestPanel QLabel#backtestPnlPlaceholder {"
        "  color:#9CA3AF;"
        "  font-size:12px;"
        "}"
        "#backtestPanel QLabel#backtestOpenNote {"
        "  color:#B45309;"
        "  font-size:11px;"
        "  padding:4px 0 0 0;"
        "}"
        "#backtestPanel QLabel#backtestSignalSummary {"
        "  color:#4B5563;"
        "  font-size:12px;"
        "  padding-top:4px;"
        "  border-top:1px solid #E5E7EB;"
        "}");
}

void applyTransparentSurface(QWidget *widget)
{
    widget->setAutoFillBackground(false);
}

void setupTimeLineEdit(QLineEdit *edit)
{
    edit->setObjectName(QStringLiteral("backtestTimeInput"));
    edit->setPlaceholderText(kTimeFormat);
    edit->setClearButtonEnabled(false);
    edit->setMaxLength(16);
    edit->setFrame(false);
    edit->setAttribute(Qt::WA_MacShowFocusRect, false);
    edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    edit->setFixedHeight(36);
}

void setStatusError(QLabel *label, bool error)
{
    label->setProperty("error", error);
    label->style()->unpolish(label);
    label->style()->polish(label);
}

void setReturnTone(QLabel *label, bool positive, bool neutral)
{
    label->setProperty("positive", !neutral && positive);
    label->setProperty("negative", !neutral && !positive);
    label->setProperty("neutral", neutral);
    label->style()->unpolish(label);
    label->style()->polish(label);
}

} // namespace

BacktestPanel::BacktestPanel(QWidget *parent)
    : QWidget(parent)
    , m_strategy(new StrategyPicker(this))
    , m_startTime(new QLineEdit(this))
    , m_endTime(new QLineEdit(this))
    , m_rangeHint(new QLabel(this))
    , m_statusLabel(new QLabel(this))
    , m_runButton(new QPushButton(tr("执行回测"), this))
    , m_pnlCard(new QFrame(this))
    , m_pnlStack(nullptr)
    , m_returnValue(nullptr)
    , m_initialCapitalValue(nullptr)
    , m_finalEquityValue(nullptr)
    , m_tradesValue(nullptr)
    , m_openPositionNote(nullptr)
    , m_signalSummary(nullptr)
{
    setObjectName(QStringLiteral("backtestPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(kPanelWidth);
    setMaximumWidth(kPanelWidth);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setStyleSheet(panelStyleSheet());

    setupTimeLineEdit(m_startTime);
    setupTimeLineEdit(m_endTime);

    m_rangeHint->setStyleSheet(QStringLiteral("color:#888;font-size:12px;"));
    m_rangeHint->setWordWrap(true);

    m_statusLabel->setObjectName(QStringLiteral("backtestStatusLabel"));
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();

    m_pnlCard->setObjectName(QStringLiteral("backtestPnlCard"));
    m_pnlCard->setAttribute(Qt::WA_StyledBackground, true);
    m_pnlCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_pnlCard->setMinimumHeight(210);

    m_returnValue = new QLabel(m_pnlCard);
    m_initialCapitalValue = makeMetricValue(QStringLiteral("backtestMetricValue"), m_pnlCard);
    m_finalEquityValue = makeMetricValue(QStringLiteral("backtestMetricValue"), m_pnlCard);
    m_tradesValue = makeMetricValue(QStringLiteral("backtestMetricValue"), m_pnlCard);
    m_openPositionNote = new QLabel(m_pnlCard);
    m_signalSummary = new QLabel(m_pnlCard);

    m_returnValue->setObjectName(QStringLiteral("backtestReturnValue"));
    m_returnValue->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    applyTransparentSurface(m_returnValue);

    m_openPositionNote->setObjectName(QStringLiteral("backtestOpenNote"));
    m_openPositionNote->setWordWrap(true);
    m_openPositionNote->hide();
    applyTransparentSurface(m_openPositionNote);

    m_signalSummary->setObjectName(QStringLiteral("backtestSignalSummary"));
    m_signalSummary->setWordWrap(true);
    applyTransparentSurface(m_signalSummary);

    auto *pnlTitle = new QLabel(tr("回测收益"), m_pnlCard);
    pnlTitle->setObjectName(QStringLiteral("backtestPnlSectionTitle"));
    applyTransparentSurface(pnlTitle);

    auto *returnCaption = new QLabel(tr("总收益率"), m_pnlCard);
    returnCaption->setObjectName(QStringLiteral("backtestReturnCaption"));
    applyTransparentSurface(returnCaption);

    auto *placeholderPage = new QWidget(m_pnlCard);
    placeholderPage->setObjectName(QStringLiteral("backtestPnlBody"));
    applyTransparentSurface(placeholderPage);
    m_pnlPlaceholder = new QLabel(tr("回测完成后在此显示收益与交易统计"), placeholderPage);
    m_pnlPlaceholder->setObjectName(QStringLiteral("backtestPnlPlaceholder"));
    m_pnlPlaceholder->setWordWrap(true);
    m_pnlPlaceholder->setAlignment(Qt::AlignCenter);
    applyTransparentSurface(m_pnlPlaceholder);
    auto *placeholderLayout = new QVBoxLayout(placeholderPage);
    placeholderLayout->setContentsMargins(0, 0, 0, 0);
    placeholderLayout->addStretch(1);
    placeholderLayout->addWidget(m_pnlPlaceholder);
    placeholderLayout->addStretch(1);

    auto *resultPage = new QWidget(m_pnlCard);
    resultPage->setObjectName(QStringLiteral("backtestPnlBody"));
    applyTransparentSurface(resultPage);
    m_metricsContainer = new QWidget(resultPage);
    m_metricsContainer->setObjectName(QStringLiteral("backtestMetricRow"));
    applyTransparentSurface(m_metricsContainer);
    auto *metricsLayout = new QVBoxLayout(m_metricsContainer);
    metricsLayout->setContentsMargins(0, 0, 0, 0);
    metricsLayout->setSpacing(8);
    metricsLayout->addWidget(makeMetricRow(tr("期初资金"), m_initialCapitalValue, m_metricsContainer));
    metricsLayout->addWidget(makeMetricRow(tr("期末资产"), m_finalEquityValue, m_metricsContainer));
    metricsLayout->addWidget(makeMetricRow(tr("完整交易"), m_tradesValue, m_metricsContainer));

    auto *resultLayout = new QVBoxLayout(resultPage);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(0);
    resultLayout->addWidget(returnCaption);
    resultLayout->addWidget(m_returnValue);
    resultLayout->addSpacing(6);
    resultLayout->addWidget(m_metricsContainer);
    resultLayout->addWidget(m_openPositionNote);
    resultLayout->addSpacing(8);
    resultLayout->addWidget(m_signalSummary);
    resultLayout->addStretch(1);

    m_pnlStack = new QStackedWidget(m_pnlCard);
    applyTransparentSurface(m_pnlStack);
    m_pnlStack->setMinimumHeight(150);
    m_pnlStack->addWidget(placeholderPage);
    m_pnlStack->addWidget(resultPage);

    auto *pnlLayout = new QVBoxLayout(m_pnlCard);
    pnlLayout->setContentsMargins(14, 14, 14, 14);
    pnlLayout->setSpacing(0);
    pnlLayout->addWidget(pnlTitle);
    pnlLayout->addSpacing(10);
    pnlLayout->addWidget(m_pnlStack, 1);
    m_pnlStack->setCurrentIndex(0);

    auto *title = new QLabel(tr("量化回测"), this);
    title->setStyleSheet(QStringLiteral("font-size:15px;font-weight:600;color:#222;"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addWidget(makeFieldLabel(tr("策略"), this));
    layout->addWidget(m_strategy);
    layout->addWidget(makeFieldLabel(tr("开始时间"), this));
    layout->addWidget(m_startTime);
    layout->addWidget(makeFieldLabel(tr("结束时间"), this));
    layout->addWidget(m_endTime);
    layout->addWidget(m_rangeHint);
    layout->addWidget(m_runButton);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_pnlCard, 1);

    showPnlCard(true);
    setPnlPlaceholder(tr("回测完成后在此显示收益与交易统计"));

    connect(m_runButton, &QPushButton::clicked, this, &BacktestPanel::onRunClicked);
    connect(m_startTime, &QLineEdit::editingFinished, this, &BacktestPanel::onTimeEditingFinished);
    connect(m_endTime, &QLineEdit::editingFinished, this, &BacktestPanel::onTimeEditingFinished);
}

QLabel *BacktestPanel::makeFieldLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("backtestFieldLabel"));
    return label;
}

QLabel *BacktestPanel::makeMetricValue(const QString &objectName, QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setObjectName(objectName);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    applyTransparentSurface(label);
    return label;
}

QWidget *BacktestPanel::makeMetricRow(const QString &caption, QLabel *valueLabel, QWidget *parent)
{
    auto *row = new QWidget(parent);
    row->setObjectName(QStringLiteral("backtestMetricRow"));
    applyTransparentSurface(row);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    auto *cap = new QLabel(caption, row);
    cap->setObjectName(QStringLiteral("backtestMetricCaption"));
    applyTransparentSurface(cap);
    cap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    valueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    rowLayout->addWidget(cap, 0, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(valueLabel, 1, Qt::AlignRight | Qt::AlignVCenter);
    return row;
}

void BacktestPanel::showPnlCard(bool show)
{
    m_pnlCard->setVisible(show);
}

void BacktestPanel::setPnlPlaceholder(const QString &text)
{
    m_pnlPlaceholder->setText(text);
    m_pnlStack->setCurrentIndex(0);
}

void BacktestPanel::setTimeEditText(QLineEdit *edit, qint64 tsSec)
{
    edit->blockSignals(true);
    edit->setText(formatTs(tsSec));
    edit->setProperty("invalid", false);
    edit->style()->unpolish(edit);
    edit->style()->polish(edit);
    edit->blockSignals(false);
}

qint64 BacktestPanel::parseTimeEdit(const QLineEdit *edit, bool *ok) const
{
    const QString text = edit->text().trimmed();
    if (text.isEmpty()) {
        if (ok) {
            *ok = false;
        }
        return -1;
    }
    QDateTime dt = QDateTime::fromString(text, kTimeFormat);
    if (dt.isValid()) {
        dt.setTimeZone(kTz);
    }
    if (!dt.isValid()) {
        if (ok) {
            *ok = false;
        }
        return -1;
    }
    if (ok) {
        *ok = true;
    }
    return dt.toSecsSinceEpoch();
}

void BacktestPanel::clampTimeInputs()
{
    if (m_minTs <= 0 || m_maxTs < m_minTs) {
        return;
    }

    auto normalize = [this](QLineEdit *edit, qint64 fallbackTs) {
        bool ok = false;
        qint64 ts = parseTimeEdit(edit, &ok);
        if (!ok) {
            edit->setProperty("invalid", true);
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);
            ts = fallbackTs;
        } else {
            edit->setProperty("invalid", false);
            edit->style()->unpolish(edit);
            edit->style()->polish(edit);
        }
        ts = qBound(m_minTs, ts, m_maxTs);
        setTimeEditText(edit, ts);
        return ts;
    };

    qint64 startTs = normalize(m_startTime, m_minTs);
    qint64 endTs = normalize(m_endTime, m_maxTs);

    if (startTs > endTs) {
        if (sender() == m_startTime) {
            endTs = startTs;
            setTimeEditText(m_endTime, endTs);
        } else {
            startTs = endTs;
            setTimeEditText(m_startTime, startTs);
        }
    }
}

void BacktestPanel::onTimeEditingFinished()
{
    clampTimeInputs();
}

void BacktestPanel::applyDateTimeLimits()
{
    if (m_minTs <= 0 || m_maxTs < m_minTs) {
        m_runButton->setEnabled(false);
        return;
    }

    clampTimeInputs();
    m_runButton->setEnabled(true);
}

void BacktestPanel::setStrategies(const QVector<StrategyRow> &strategies)
{
    m_strategy->setStrategies(strategies);
}

void BacktestPanel::setFileTimeRange(qint64 minTsSec, qint64 maxTsSec)
{
    m_fileMinTs = minTsSec;
    m_fileMaxTs = maxTsSec;

    if (minTsSec <= 0 || maxTsSec < minTsSec) {
        m_minTs = 0;
        m_maxTs = 0;
        m_runButton->setEnabled(false);
        updateRangeLabel();
        return;
    }

    m_minTs = minTsSec;
    m_maxTs = maxTsSec;

    setTimeEditText(m_startTime, m_minTs);
    setTimeEditText(m_endTime, m_maxTs);

    applyDateTimeLimits();
    updateRangeLabel();
}

void BacktestPanel::expandLoadedRange(qint64 minTsSec, qint64 maxTsSec)
{
    if (minTsSec <= 0 || maxTsSec < minTsSec) {
        return;
    }

    if (m_fileMinTs > 0 && m_fileMaxTs >= m_fileMinTs) {
        minTsSec = qMax(minTsSec, m_fileMinTs);
        maxTsSec = qMin(maxTsSec, m_fileMaxTs);
        if (maxTsSec < minTsSec) {
            return;
        }
    }

    bool changed = false;
    if (m_minTs <= 0 || minTsSec < m_minTs) {
        m_minTs = minTsSec;
        changed = true;
    }
    if (m_maxTs < maxTsSec) {
        m_maxTs = maxTsSec;
        changed = true;
    }

    if (m_minTs <= 0) {
        m_minTs = minTsSec;
        m_maxTs = maxTsSec;
        setTimeEditText(m_startTime, m_minTs);
        setTimeEditText(m_endTime, m_maxTs);
        changed = true;
    }

    if (changed) {
        applyDateTimeLimits();
    }
    updateRangeLabel();
}

void BacktestPanel::setRunning(bool running)
{
    m_runButton->setEnabled(!running && m_minTs > 0 && m_maxTs >= m_minTs);
    m_strategy->setEnabled(!running);
    m_startTime->setEnabled(!running);
    m_endTime->setEnabled(!running);
}

void BacktestPanel::setBacktestResult(const BacktestSummary &summary, int buyCount, int sellCount)
{
    setStatusError(m_statusLabel, false);
    m_statusLabel->clear();
    m_statusLabel->hide();

    const QLocale locale = QLocale::system();
    const bool neutral = qAbs(summary.totalReturnPct) < 0.005;
    m_returnValue->setText(formatReturnPct(locale, summary.totalReturnPct));
    setReturnTone(m_returnValue, summary.totalReturnPct >= 0.0, neutral);

    m_initialCapitalValue->setText(formatMoney(locale, summary.initialCapital));
    m_finalEquityValue->setText(formatMoney(locale, summary.finalEquity));
    m_tradesValue->setText(tr("%1 笔 · 盈 %2 / 亏 %3")
                               .arg(summary.roundTrips)
                               .arg(summary.winCount)
                               .arg(summary.lossCount));

    if (summary.openPosition) {
        m_openPositionNote->setText(tr("期末仍持仓，已按区间末收盘价估算权益"));
        m_openPositionNote->show();
    } else {
        m_openPositionNote->hide();
    }

    m_signalSummary->setText(tr("信号：买入 %1 次，卖出 %2 次").arg(buyCount).arg(sellCount));
    m_pnlStack->setCurrentIndex(1);
}

void BacktestPanel::setResultText(const QString &text)
{
    const bool isError = text.contains(tr("失败")) || text.contains(QStringLiteral("格式"))
                         || text.contains(tr("不能")) || text.contains(tr("须在"));
    if (isError) {
        setStatusError(m_statusLabel, true);
        m_statusLabel->setText(text);
        m_statusLabel->show();
        setPnlPlaceholder(tr("请修正参数后重新回测"));
        return;
    }
    setStatusError(m_statusLabel, false);
    m_statusLabel->clear();
    m_statusLabel->hide();
    setPnlPlaceholder(text);
}

void BacktestPanel::onRunClicked()
{
    clampTimeInputs();

    bool startOk = false;
    bool endOk = false;
    const qint64 startTs = parseTimeEdit(m_startTime, &startOk);
    const qint64 endTs = parseTimeEdit(m_endTime, &endOk);

    if (!startOk || !endOk) {
        setResultText(tr("时间格式应为 yyyy-MM-dd HH:mm，例如 2024-01-02 09:30"));
        return;
    }

    if (startTs < m_minTs || startTs > m_maxTs || endTs < m_minTs || endTs > m_maxTs) {
        setResultText(tr("时间须在可选范围内：%1 ～ %2").arg(formatTs(m_minTs), formatTs(m_maxTs)));
        return;
    }

    if (startTs > endTs) {
        setResultText(tr("开始时间不能晚于结束时间"));
        return;
    }

    setStatusError(m_statusLabel, false);
    m_statusLabel->clear();
    m_statusLabel->hide();
    emit runRequested(m_strategy->currentStrategyId(), startTs, endTs);
}

void BacktestPanel::updateRangeLabel()
{
    if (m_minTs <= 0 || m_maxTs < m_minTs) {
        m_rangeHint->setText(tr("请先加载 K 线数据"));
        return;
    }
    m_rangeHint->setText(
        tr("输入格式：yyyy-MM-dd HH:mm\n可选范围：%1 ～ %2")
            .arg(formatTs(m_minTs), formatTs(m_maxTs)));
    if (m_fileMinTs > 0 && m_fileMaxTs >= m_fileMinTs
        && (m_fileMinTs != m_minTs || m_fileMaxTs != m_maxTs)) {
        m_rangeHint->setText(
            tr("输入格式：yyyy-MM-dd HH:mm\n文件范围：%1 ～ %2\n当前可选：%3 ～ %4")
                .arg(formatTs(m_fileMinTs), formatTs(m_fileMaxTs), formatTs(m_minTs),
                     formatTs(m_maxTs)));
    }
}
