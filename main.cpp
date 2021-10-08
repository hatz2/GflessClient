#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Hatz Nostale");
    QCoreApplication::setApplicationName("Gfless Client");
    MainWindow w;
    w.show();
    return a.exec();
}
