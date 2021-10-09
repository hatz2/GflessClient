#include "MainWindow.h"
#include "ProcessChecker.h"

#include <QApplication>
#include <QMessageBox>

void checkGameforgeClient();

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    checkGameforgeClient();
    QCoreApplication::setOrganizationName("Hatz Nostale");
    QCoreApplication::setApplicationName("Gfless Client");
    MainWindow w;
    w.show();
    return a.exec();
}

void checkGameforgeClient()
{
    const wchar_t* gameforgeClientName = L"gfclient.exe";

    if (isProcessRunning(gameforgeClientName))
    {
        int res = QMessageBox::critical(nullptr, "Error", "Seems like Gameforge Client is running.\nDo you want to kill the process?", QMessageBox::Yes | QMessageBox::No);

        if (res == QMessageBox::Yes)
            killProcess(gameforgeClientName);

        else
            exit(EXIT_FAILURE);
    }
}
