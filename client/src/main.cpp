#include "app/AppBranding.h"
#include "app/AppStyle.h"
#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    AppStyle::applyFixedLightTheme(app);
    QApplication::setApplicationName(QStringLiteral("MockTrader"));
    QApplication::setOrganizationName(QStringLiteral("MockTrader"));
    QApplication::setWindowIcon(AppBranding::applicationIcon());

    MainWindow w;
    w.resize(1100, 720);
    w.show();

    return app.exec();
}
