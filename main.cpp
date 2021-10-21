#include "MainWindow.h"
#include "ProcessChecker.h"

#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>

void checkGameforgeClient();

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s \n", localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s \n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s \n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s \n", localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s \n", localMsg.constData());
        abort();
    }
}

int main(int argc, char *argv[])
{
    AllocConsole();
    FILE* file = new FILE;
    freopen_s(&file, "CONOUT$", "w", stderr);

    qInstallMessageHandler(myMessageOutput); // Install the handler
    QApplication a(argc, argv);

    // Check if another instance of the app is already running
    QSharedMemory sharedMemory("Gfless Client");
    if (!sharedMemory.create(1))
    {
        QMessageBox::critical(nullptr, "Error", "Gfless Client is already running.");
        return 0;
    }

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
            exit(0);
    }
}
