#ifndef GFLESSCLIENT_H
#define GFLESSCLIENT_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <QSettings>
#include <QTimer>
#include <Windows.h>

class GflessClient : public QObject
{
    Q_OBJECT
public:
    explicit GflessClient(QObject *parent = nullptr);

    bool openClient(const QString& displayName, const QString& token, const QString& gameClientPath, const int& gameLanguage, DWORD& createdPID);

    bool openClientSettings(const QString& gameClientPath);

    // The real Gameforge client refuses to start ("already running") while
    // this pipe exists, so it is released while the Gameforge client has to
    // run (the device identity capture) and taken back afterwards.
    void releaseGameforgePipe();

    // Listens on the pipe again; false while another process still owns it
    // (the Gameforge client is still open). openClient retries by itself.
    bool restoreGameforgePipe();

private slots:
    void handleNewConnection();

    void handlePipe();

private:
    QLocalServer* gfServer;
    QLocalSocket* pipe;
    QString token;
    QString displayName;

    QByteArray prepareResponse(const QJsonObject& request, const QString& response);

    bool injectDll(DWORD pid, const QString& dllPath);
};

#endif // GFLESSCLIENT_H
