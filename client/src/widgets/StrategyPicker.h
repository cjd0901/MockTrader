#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class QMenu;
class QMouseEvent;
class QKeyEvent;

/// 自定义策略选择（无原生 QComboBox 下拉，风格与时间输入一致）。
class StrategyPicker final : public QFrame
{
    Q_OBJECT

public:
    explicit StrategyPicker(QWidget *parent = nullptr);

    QString currentStrategyId() const { return m_currentId; }

public slots:
    void setCurrentStrategy(const QString &strategyId);

signals:
    void strategyChanged(const QString &strategyId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void openMenu();
    void applyOpenState(bool open);

    QLabel *m_label = nullptr;
    QLabel *m_chevron = nullptr;
    QMenu *m_menu = nullptr;
    QString m_currentId;
};
