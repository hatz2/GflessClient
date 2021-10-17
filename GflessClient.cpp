#include "GflessClient.h"
#include "Injector.h"

GflessClient::GflessClient(QObject *parent) : QObject(parent)
{
    pipe = nullptr;
    gfServer = new QLocalServer(this);
    gfServer->listen("GameforgeClientJSONRPC");
    token = "";
    displayName = "";

    setEnvironmentVariables();

    connect(gfServer, &QLocalServer::newConnection, this, &GflessClient::handleNewConnection);
}

bool GflessClient::openClient(const QString& displayName, const QString& token, const QString &gameClientPath, const int &gameLanguage, bool autoLogin)
{
    this->displayName = displayName;
    this->token = token;

    int index = gameClientPath.lastIndexOf("/NostaleClientX.exe");

    if (index < 0)
        return false;

    QString directory = gameClientPath.left(index);
    qint64 pid;

    if (!QProcess::startDetached(gameClientPath, {"gf", QString::number(gameLanguage)}, directory, &pid))
        return false;

    if (autoLogin)
    {
        QString dllPath = QDir::currentPath() + "/NostaleLogin.dll";

        if (!Inject(pid, dllPath.toLocal8Bit().data()))
            qDebug() << "Dll injection failed";
    }

    return true;
}

bool GflessClient::openClientSettings(const QString &gameClientPath)
{
    int index = gameClientPath.lastIndexOf("/NostaleClientX.exe");

    if (index < 0)
        return false;

    QString directory = gameClientPath.left(index);
    QString settingsPath = directory + "/NtConfig.exe";

    return QProcess::startDetached(settingsPath, {}, directory);
}

void GflessClient::handleNewConnection()
{
    pipe = gfServer->nextPendingConnection();
    connect(pipe, &QLocalSocket::readyRead, this, &GflessClient::handlePipe);
}

void GflessClient::handlePipe()
{
    QByteArray output;
    QJsonObject json = QJsonDocument::fromJson(pipe->readAll()).object();
    QString method = json["method"].toString();

    qDebug() << QJsonDocument(json).toJson();

    if (method == "ClientLibrary.isClientRunning")
        output = prepareResponse(json, "true");

    else if  (method == "ClientLibrary.initSession")
        output = prepareResponse(json, json["params"].toObject()["sessionId"].toString().toUtf8());

    else if (method == "ClientLibrary.queryAuthorizationCode")
        output = prepareResponse(json, token.toUtf8());

    else if (method == "ClientLibrary.queryGameAccountName")
        output = prepareResponse(json, displayName.toUtf8());

    else
        return;

    pipe->write(output);
}

QByteArray GflessClient::prepareResponse(const QJsonObject &request, const QString &response)
{
    QJsonObject resp;

    resp["id"] = request["id"];
    resp["jsonrpc"] = request["jsonrpc"];
    resp["result"] = response;

    return QJsonDocument(resp).toJson();
}

void GflessClient::setEnvironmentVariables() const
{
    QSettings settings("HKEY_CURRENT_USER\\Environment", QSettings::NativeFormat);
    settings.setValue("_TNT_CLIENT_APPLICATION_ID", "d3b2a0c1-f0d0-4888-ae0b-1c5e1febdafb");
    settings.setValue("_TNT_SESSION_ID", "12345678-1234-1234-1234-123456789012");
}
