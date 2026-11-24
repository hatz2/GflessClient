#include "gflessclient.h"
#include <QMessageBox>
#include <QUuid>

GflessClient::GflessClient(QObject *parent) : QObject(parent)
{
    gfServer = new QLocalServer(this);
    gfServer->listen("GameforgeClientJSONRPC");

    connect(gfServer, &QLocalServer::newConnection, this, &GflessClient::handleNewConnection);
}

bool GflessClient::openClient(const QString& displayName, const QString& token, const QString &gameClientPath, const int &gameLanguage, DWORD& createdPID, QString& sessionId)
{
    // Every launched game gets its own session id (like the real launcher's
    // random _TNT_SESSION_ID) and its own code slot, so concurrent clients
    // can never receive each other's one-time code.
    sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    sessions.insert(sessionId, {token, displayName});
    sessionOrder.append(sessionId);
    while (sessionOrder.size() > 50) // bounded; the game always self-identifies
        sessions.remove(sessionOrder.takeFirst());

    int index = gameClientPath.lastIndexOf("/");

    if (index < 0)
        return false;

    QString directory = gameClientPath.left(index);
    QProcess process;
    QProcessEnvironment env = process.processEnvironment();
    qint64 pid;

    env.insert("_TNT_CLIENT_APPLICATION_ID", "d3b2a0c1-f0d0-4888-ae0b-1c5e1febdafb");
    env.insert("_TNT_SESSION_ID", sessionId);

    process.setProcessEnvironment(env);
    process.setWorkingDirectory(directory);
    process.setProgram(gameClientPath);
    process.setArguments({"gf", QString::number(gameLanguage)});

    PROCESS_INFORMATION* procInfo;
    DWORD threadId;

    process.setCreateProcessArgumentsModifier([&](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_SUSPENDED;
        procInfo = args->processInformation;
    });

    if (!process.startDetached(&pid))
    {
        qDebug() << "Error creating process";
        return false;
    }

    threadId = procInfo->dwThreadId;

    QString dllPath = QDir::currentPath() + "/GflessDLL.dll";

    if (!injectDll(pid, dllPath))
        qDebug() << "Dll injection failed";
    else
        qDebug() << "Dll injected successfully";

    // Resume the game
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);

    if (hThread == NULL)
    {
        qDebug() << "Error on OpenThread:" << GetLastError();
        return false;
    }

    if (ResumeThread(hThread) == -1) {
        qDebug() << "Error on ResumeThread:" << GetLastError();
        CloseHandle(hThread);
        return false;
    }

    CloseHandle(hThread);
    createdPID = pid;

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
    QLocalSocket* sock = gfServer->nextPendingConnection();
    connect(sock, &QLocalSocket::readyRead, this, [this, sock]() { handlePipe(sock); });
    // NOTE: sessions are deliberately NOT removed here. The game opens and
    // closes pipe connections during its handshake; wiping the code slot on
    // disconnect is what starved later queries of their answer.
    connect(sock, &QLocalSocket::disconnected, sock, &QLocalSocket::deleteLater);
}

void GflessClient::handlePipe(QLocalSocket* sock)
{
    QByteArray output;
    QJsonObject json = QJsonDocument::fromJson(sock->readAll()).object();
    QString method = json["method"].toString();

    qDebug() << QJsonDocument(json).toJson();

    if (method == "ClientLibrary.isClientRunning")
        output = prepareResponse(json, "true");

    else if  (method == "ClientLibrary.initSession")
        output = prepareResponse(json, json["params"].toObject()["sessionId"].toString().toUtf8());

    else if (method == "ClientLibrary.queryAuthorizationCode")
    {
        const QString sid = sessionFor(sock, json);
        if (sid.isEmpty())
            return; // never answer with another client's code
        output = prepareResponse(json, sessions[sid].token.toUtf8());
    }

    else if (method == "ClientLibrary.queryGameAccountName")
    {
        const QString sid = sessionFor(sock, json);
        if (sid.isEmpty())
            return;
        output = prepareResponse(json, sessions[sid].displayName.toUtf8());
    }

    else
        return;

    sock->write(output);
}

QString GflessClient::sessionFor(QLocalSocket* sock, const QJsonObject& json)
{
    // Preferred: the session id the game itself reports (present in the
    // isClientRunning/initSession params, like the real launcher flow).
    QString sid = json["params"].toObject()["sessionId"].toString();
    if (!sid.isEmpty())
    {
        sock->setProperty("sessionId", sid);
        if (sessions.contains(sid))
            return sid;
    }

    // Otherwise stick to this socket's session (one connection per game).
    sid = sock->property("sessionId").toString();
    if (!sid.isEmpty() && sessions.contains(sid))
        return sid;

    // Last resort: exactly one client waiting.
    if (sessions.size() == 1)
        return sessions.firstKey();

    // Several clients waiting: oldest one. (First choice is always the
    // session id the game itself reports; this is only the fallback for
    // messages without one.)
    for (const QString& key : sessionOrder)
        if (sessions.contains(key))
            return key;

    return QString();
}

QByteArray GflessClient::prepareResponse(const QJsonObject &request, const QString &response)
{
    QJsonObject resp;

    resp["id"] = request["id"];
    resp["jsonrpc"] = request["jsonrpc"];
    resp["result"] = response;

    return QJsonDocument(resp).toJson();
}

bool GflessClient::injectDll(DWORD pid, const QString &dllPath)
{
    QString injectorPath = QDir::currentPath() + "/Injector.exe";
    QStringList arguments;
    arguments << QString::number(pid) << dllPath;

    if (!QFile::exists(injectorPath)) {
        QMessageBox::critical((QWidget*)this->parent(), "Error", "Injector.exe not found in the current directory.\nMake sure your antivirus has not deleted it");
        return false;
    }

    QProcess process;
    process.start(injectorPath, arguments);

    process.waitForFinished();

    qDebug() << process.readAllStandardOutput();

    return !process.exitCode();
}
