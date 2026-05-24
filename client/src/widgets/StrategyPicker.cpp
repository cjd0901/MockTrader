#include "widgets/StrategyPicker.h"

#include <QAction>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QStyle>

#include <algorithm>

StrategyPicker::StrategyPicker(QWidget *parent)
    : QFrame(parent)
    , m_label(new QLabel(this))
    , m_chevron(new QLabel(QStringLiteral("▾"), this))
    , m_menu(new QMenu(this))
{
    setObjectName(QStringLiteral("backtestStrategyPicker"));
    setFrameShape(QFrame::NoFrame);
    setLineWidth(0);
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(36);

    m_label->setObjectName(QStringLiteral("strategyPickerLabel"));
    m_chevron->setObjectName(QStringLiteral("strategyPickerChevron"));
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_label->setFocusPolicy(Qt::NoFocus);
    m_chevron->setFocusPolicy(Qt::NoFocus);

    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(10, 0, 10, 0);
    row->setSpacing(6);
    row->addWidget(m_label, 1);
    row->addWidget(m_chevron, 0, Qt::AlignRight | Qt::AlignVCenter);

    m_menu->setObjectName(QStringLiteral("backtestStrategyMenu"));
    connect(m_menu, &QMenu::triggered, this, [this](QAction *action) {
        if (!action) {
            return;
        }
        const QString id = action->data().toString();
        if (id != m_currentId) {
            m_currentId = id;
            m_label->setText(action->text());
            emit strategyChanged(m_currentId);
        }
        applyOpenState(false);
    });
    connect(m_menu, &QMenu::aboutToHide, this, [this]() { applyOpenState(false); });

    m_label->setText(tr("加载策略…"));
}

void StrategyPicker::setStrategies(const QVector<StrategyRow> &strategies)
{
    m_strategies = strategies;
    m_menu->clear();

    for (const StrategyRow &row : m_strategies) {
        QAction *action = m_menu->addAction(row.displayName);
        action->setData(row.id);
    }

    if (m_strategies.isEmpty()) {
        m_currentId.clear();
        m_label->setText(tr("无可用策略"));
        return;
    }

    if (m_currentId.isEmpty()
        || !std::any_of(m_strategies.cbegin(), m_strategies.cend(),
                        [this](const StrategyRow &r) { return r.id == m_currentId; })) {
        setCurrentStrategy(m_strategies.first().id);
    } else {
        setCurrentStrategy(m_currentId);
    }
}

void StrategyPicker::setCurrentStrategy(const QString &strategyId)
{
    for (const StrategyRow &row : m_strategies) {
        if (strategyId == row.id) {
            m_currentId = strategyId;
            m_label->setText(row.displayName);
            return;
        }
    }
    if (!m_strategies.isEmpty()) {
        m_currentId = m_strategies.first().id;
        m_label->setText(m_strategies.first().displayName);
    }
}

void StrategyPicker::applyOpenState(bool open)
{
    setProperty("open", open);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void StrategyPicker::openMenu()
{
    if (m_strategies.isEmpty()) {
        return;
    }
    applyOpenState(true);
    m_menu->setFixedWidth(width());
    const QPoint pos = mapToGlobal(QPoint(0, height() + 2));
    m_menu->popup(pos);
}

void StrategyPicker::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        openMenu();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void StrategyPicker::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        openMenu();
        event->accept();
        return;
    }
    QFrame::keyPressEvent(event);
}
