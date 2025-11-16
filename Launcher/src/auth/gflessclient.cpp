#include "gflessclient.h"
#include <QMessageBox>

GflessClient::GflessClient(QObject *parent) : QObject(parent)
{
    pipe = nullptr;
    gfServer = new QLocalServer(this);
    gfServer->listen("GameforgeClientJSONRPC");
    token = "";
    displayName = "";

    connect(gfServer, &QLocalServer::newConnection, this, &GflessClient::handleNewConnection);
}

bool GflessClient::openClient(const QString& displayName, const QString& token, const QString &gameClientPath, const int &gameLanguage, DWORD& createdPID)
{
    this->displayName = displayName;
    this->token = token;

    int index = gameClientPath.lastIndexOf("/");

    if (index < 0)
        return false;

    QString directory = gameClientPath.left(index);
    QProcess process;
    QProcessEnvironment env = process.processEnvironment();
    qint64 pid;

    env.insert("_TNT_CLIENT_APPLICATION_ID", "d3b2a0c1-f0d0-4888-ae0b-1c5e1febdafb");
    env.insert("_TNT_SESSION_ID", "12345678-1234-1234-1234-123456789012");

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
    // Support both NosTale and Metin2
    int index = gameClientPath.lastIndexOf("/NostaleClientX.exe");

    if (index < 0) {
        // Try Metin2 client
        index = gameClientPath.lastIndexOf("/metin2client.exe");
    }

    if (index < 0)
        return false;

    QString directory = gameClientPath.left(index);

    // Check if it's NosTale or Metin2
    if (gameClientPath.contains("NostaleClientX.exe", Qt::CaseInsensitive)) {
        QString settingsPath = directory + "/NtConfig.exe";
        return QProcess::startDetached(settingsPath, {}, directory);
    } else {
        // Metin2 - settings may vary, check common config executables
        QString settingsPath = directory + "/metin2config.exe";
        if (QFile::exists(settingsPath)) {
            return QProcess::startDetached(settingsPath, {}, directory);
        }
        // If no config found, just return false
        return false;
    }
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
