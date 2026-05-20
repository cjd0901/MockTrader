#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("MockTrader"));
    QApplication::setOrganizationName(QStringLiteral("MockTrader"));

    MainWindow w;
    w.resize(1024, 640);
    w.show();

    return app.exec();
}
