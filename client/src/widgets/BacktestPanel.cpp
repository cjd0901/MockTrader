#include "widgets/BacktestPanel.h"

#include "widgets/StrategyPicker.h"

#include <QDateTime>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QStyle>
#include <QTimeZone>
#include <QVBoxLayout>

namespace {

const QTimeZone kTz(QByteArrayLiteral("Asia/Shanghai"));
const QString kTimeFormat = QStringLiteral("yyyy-MM-dd HH:mm");

QString formatTs(qint64 tsSec)
{
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(tsSec, kTz);
    return dt.isValid() ? dt.toString(kTimeFormat) : QStringLiteral("--");
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
        "#backtestPanel QPushButton:hover:!disabled { background:#3367D6; }");
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

} // namespace

BacktestPanel::BacktestPanel(QWidget *parent)
    : QWidget(parent)
    , m_strategy(new StrategyPicker(this))
    , m_startTime(new QLineEdit(this))
    , m_endTime(new QLineEdit(this))
    , m_rangeHint(new QLabel(this))
    , m_pnlTitle(new QLabel(this))
    , m_pnlLabel(new QLabel(this))
    , m_resultLabel(new QLabel(this))
    , m_runButton(new QPushButton(tr("执行回测"), this))
{
    setObjectName(QStringLiteral("backtestPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(280);
    setStyleSheet(panelStyleSheet());

    setupTimeLineEdit(m_startTime);
    setupTimeLineEdit(m_endTime);

    m_rangeHint->setStyleSheet(QStringLiteral("color:#888;font-size:12px;"));
    m_rangeHint->setWordWrap(true);

    m_pnlTitle->setText(tr("回测收益"));
    m_pnlTitle->setStyleSheet(QStringLiteral("font-size:13px;font-weight:600;color:#222;"));
    m_pnlLabel->setStyleSheet(QStringLiteral("color:#333;font-size:12px;line-height:1.5;"));
    m_pnlLabel->setWordWrap(true);
    m_pnlLabel->hide();
    m_pnlTitle->hide();

    m_resultLabel->setStyleSheet(QStringLiteral("color:#666;font-size:12px;"));
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setText(tr("选择时间范围后点击执行回测"));

    auto *title = new QLabel(tr("量化回测"), this);
    title->setStyleSheet(QStringLiteral("font-size:15px;font-weight:600;color:#222;"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(title);
    layout->addWidget(new QLabel(tr("策略"), this));
    layout->addWidget(m_strategy);
    layout->addWidget(new QLabel(tr("开始时间"), this));
    layout->addWidget(m_startTime);
    layout->addWidget(new QLabel(tr("结束时间"), this));
    layout->addWidget(m_endTime);
    layout->addWidget(m_rangeHint);
    layout->addWidget(m_runButton);
    layout->addWidget(m_pnlTitle);
    layout->addWidget(m_pnlLabel);
    layout->addWidget(m_resultLabel);
    layout->addStretch(1);

    connect(m_runButton, &QPushButton::clicked, this, &BacktestPanel::onRunClicked);
    connect(m_startTime, &QLineEdit::editingFinished, this, &BacktestPanel::onTimeEditingFinished);
    connect(m_endTime, &QLineEdit::editingFinished, this, &BacktestPanel::onTimeEditingFinished);
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
    if (running) {
        m_pnlLabel->hide();
        m_pnlTitle->hide();
        m_resultLabel->setStyleSheet(QStringLiteral("color:#666;font-size:12px;"));
        m_resultLabel->setText(tr("回测计算中…"));
    }
}

void BacktestPanel::setBacktestResult(const BacktestSummary &summary, int buyCount, int sellCount)
{
    m_pnlTitle->show();
    m_pnlLabel->show();

    const QLocale locale = QLocale::system();
    const QString returnText = summary.totalReturnPct >= 0.0
        ? QStringLiteral("+%1%").arg(locale.toString(summary.totalReturnPct, 'f', 2))
        : QStringLiteral("%1%").arg(locale.toString(summary.totalReturnPct, 'f', 2));
    const QColor returnColor =
        summary.totalReturnPct >= 0.0 ? QColor(0x1B, 0xAA, 0x3A) : QColor(0xF0, 0x3E, 0x3E);
    m_pnlLabel->setStyleSheet(
        QStringLiteral("color:%1;font-size:12px;line-height:1.5;").arg(returnColor.name()));

    QString pnlText = tr("总收益率：%1").arg(returnText);
    pnlText += QStringLiteral("\n");
    pnlText += tr("期初资金：%1 元")
                   .arg(locale.toString(summary.initialCapital, 'f', 2));
    pnlText += QStringLiteral("\n");
    pnlText += tr("期末资产：%1 元").arg(locale.toString(summary.finalEquity, 'f', 2));
    pnlText += QStringLiteral("\n");
    pnlText += tr("完整交易：%1 笔（盈 %2 / 亏 %3）")
                   .arg(summary.roundTrips)
                   .arg(summary.winCount)
                   .arg(summary.lossCount);
    if (summary.openPosition) {
        pnlText += QStringLiteral("\n");
        pnlText += tr("注：期末仍持仓，已按区间末收盘价估算");
    }
    m_pnlLabel->setText(pnlText);

    m_resultLabel->setStyleSheet(QStringLiteral("color:#666;font-size:12px;"));
    m_resultLabel->setText(
        tr("信号：买入 %1 次，卖出 %2 次").arg(buyCount).arg(sellCount));
}

void BacktestPanel::setResultText(const QString &text)
{
    m_pnlLabel->hide();
    m_pnlTitle->hide();
    m_resultLabel->setStyleSheet(QStringLiteral("color:#666;font-size:12px;"));
    m_resultLabel->setText(text);
}

void BacktestPanel::onRunClicked()
{
    clampTimeInputs();

    bool startOk = false;
    bool endOk = false;
    const qint64 startTs = parseTimeEdit(m_startTime, &startOk);
    const qint64 endTs = parseTimeEdit(m_endTime, &endOk);

    if (!startOk || !endOk) {
        m_resultLabel->setStyleSheet(QStringLiteral("color:#F03E3E;font-size:12px;"));
        m_resultLabel->setText(tr("时间格式应为 yyyy-MM-dd HH:mm，例如 2024-01-02 09:30"));
        return;
    }

    if (startTs < m_minTs || startTs > m_maxTs || endTs < m_minTs || endTs > m_maxTs) {
        m_resultLabel->setStyleSheet(QStringLiteral("color:#F03E3E;font-size:12px;"));
        m_resultLabel->setText(
            tr("时间须在可选范围内：%1 ～ %2").arg(formatTs(m_minTs), formatTs(m_maxTs)));
        return;
    }

    if (startTs > endTs) {
        m_resultLabel->setStyleSheet(QStringLiteral("color:#F03E3E;font-size:12px;"));
        m_resultLabel->setText(tr("开始时间不能晚于结束时间"));
        return;
    }

    m_resultLabel->setStyleSheet(QStringLiteral("color:#666;font-size:12px;"));
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
