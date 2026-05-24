#include "widgets/StrategyPicker.h"

#include <QAction>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QStyle>

namespace {

struct StrategyOption {
    const char *id;
    const char *labelUtf8;
};

const StrategyOption kStrategies[] = {
    {"macd_cross", "MACD金叉买入 / 死叉卖出"},
};

} // namespace

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
    for (const StrategyOption &opt : kStrategies) {
        QAction *action = m_menu->addAction(QString::fromUtf8(opt.labelUtf8));
        action->setData(QString::fromUtf8(opt.id));
    }
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

    setCurrentStrategy(QStringLiteral("macd_cross"));
}

void StrategyPicker::setCurrentStrategy(const QString &strategyId)
{
    for (const StrategyOption &opt : kStrategies) {
        if (strategyId == QLatin1String(opt.id)) {
            m_currentId = strategyId;
            m_label->setText(QString::fromUtf8(opt.labelUtf8));
            return;
        }
    }
    m_currentId = QString::fromUtf8(kStrategies[0].id);
    m_label->setText(QString::fromUtf8(kStrategies[0].labelUtf8));
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
