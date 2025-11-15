#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    try {
        QApplication app(argc, argv);
        MainWindow w;
        w.show();
        return app.exec();
    } catch (const std::runtime_error& e) {
        qDebug() << "Critical error:" << e.what();
        return EXIT_FAILURE;
    }
}
