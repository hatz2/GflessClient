#ifndef GFLESSCLIENT_H
#define GFLESSCLIENT_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <Windows.h>
#include <memory>

class GflessClient : public QObject
{
    Q_OBJECT
public:
    explicit GflessClient(QObject *parent = nullptr);

    bool openClient(const QString &displayName, const QString &code, const QString &gameClientPath,
                    int gameLanguage, DWORD &createdPID);

    bool openClientSettings(const QString& gameClientPath);

private slots:
    void handleNewConnection();

private:
    QLocalServer* gfServer;
    std::unique_ptr<void, decltype(&CloseHandle)> gameProcess{nullptr, CloseHandle};
    QString launchCode;
    QString displayName;

    bool isGameConnection(QLocalSocket *socket) const;
    void handlePipe(QLocalSocket *socket);
    QByteArray prepareResponse(const QJsonObject& request, const QString& response);

    bool injectDll(DWORD pid, const QString& dllPath);
};

#endif // GFLESSCLIENT_H
