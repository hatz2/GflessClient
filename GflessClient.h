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

class GflessClient : public QObject
{
    Q_OBJECT
public:
    explicit GflessClient(QObject *parent = nullptr);

    bool openClient(const QString& displayName, const QString& token, const QString& gameClientPath, const int& gameLanguage, bool autoLogin);

    bool openClientSettings(const QString& gameClientPath);

private slots:
    void handleNewConnection();

    void handlePipe();

private:
    QByteArray prepareResponse(const QJsonObject& request, const QString& response);

private:
    void setEnvironmentVariables() const;

    QLocalServer* gfServer;
    QLocalSocket* pipe;
    QString token;
    QString displayName;
};

#endif // GFLESSCLIENT_H
