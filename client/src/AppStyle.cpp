#include "AppStyle.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

namespace AppStyle {

void applyFixedLightTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette palette;
    const QColor window(0xFF, 0xFF, 0xFF);
    const QColor text(0x22, 0x22, 0x22);
    const QColor button(0xF5, 0xF5, 0xF5);
    const QColor border(0xE6, 0xE6, 0xE6);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, window);
    palette.setColor(QPalette::AlternateBase, QColor(0xFA, 0xFA, 0xFA));
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, text);
    palette.setColor(QPalette::Highlight, QColor(0x42, 0x85, 0xF4));
    palette.setColor(QPalette::HighlightedText, window);
    palette.setColor(QPalette::ToolTipBase, window);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Link, QColor(0x42, 0x85, 0xF4));
    palette.setColor(QPalette::Mid, border);
    palette.setColor(QPalette::Dark, border);
    palette.setColor(QPalette::Shadow, border);
    palette.setColor(QPalette::Light, window);

    app.setPalette(palette);
}

} // namespace AppStyle
