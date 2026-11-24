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
#include <QMap>
#include <Windows.h>

struct GfSession
{
    QString token;        // one-time game code for this client only
    QString displayName;
};

class GflessClient : public QObject
{
    Q_OBJECT
public:
    explicit GflessClient(QObject *parent = nullptr);

    bool openClient(const QString& displayName, const QString& token, const QString& gameClientPath, const int& gameLanguage, DWORD& createdPID, QString& sessionId);

    bool openClientSettings(const QString& gameClientPath);

private slots:
    void handleNewConnection();

private:
    void handlePipe(QLocalSocket* sock);
    QString sessionFor(QLocalSocket* sock, const QJsonObject& json);

    QLocalServer* gfServer;

    // One entry per launched game client, keyed by its unique _TNT_SESSION_ID.
    // Entries persist for the launcher session (a few dozen QStrings at most);
    // the game always identifies itself, so stale entries are never consulted.
    QMap<QString, GfSession> sessions;
    QStringList sessionOrder;

    QByteArray prepareResponse(const QJsonObject& request, const QString& response);

    bool injectDll(DWORD pid, const QString &dllPath);
};

#endif // GFLESSCLIENT_H
