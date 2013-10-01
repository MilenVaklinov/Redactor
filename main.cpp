#include <QtWidgets/QApplication>
#include <QStyleFactory>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

    a.setCursorFlashTime(0);

    MainWindow w;

    QFile qss(":/css/style.qss");
    qss.open(QFile::ReadOnly);
    QString StyleSheet = QLatin1String(qss.readAll());;
    qApp->setStyleSheet(StyleSheet);
    qss.close();

    w.show();

    return a.exec();
}
