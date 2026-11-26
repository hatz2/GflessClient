#include "gflessclient.h"
#include <QFileInfo>
#include <QMessageBox>
#include <string>
#include <utility>

GflessClient::GflessClient(QObject *parent) : QObject(parent)
{
    gfServer = new QLocalServer(this);
    gfServer->listen("GameforgeClientJSONRPC");

    connect(gfServer, &QLocalServer::newConnection, this, &GflessClient::handleNewConnection);
}

bool GflessClient::openClient(const QString &displayName, const QString &code, const QString &gameClientPath,
                             int gameLanguage, DWORD &createdPID)
{
    if (!gfServer->isListening())
        return false;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("_TNT_CLIENT_APPLICATION_ID", "d3b2a0c1-f0d0-4888-ae0b-1c5e1febdafb");
    env.insert("_TNT_SESSION_ID", "12345678-1234-1234-1234-123456789012");
    QStringList variables = env.toStringList();
    variables.sort(Qt::CaseInsensitive);
    std::wstring environment = variables.join(QChar(u'\0')).toStdWString();
    environment.push_back(L'\0'); // Including the string terminator, the block ends with two NULs.

    const QFileInfo game(gameClientPath);
    const std::wstring path = QDir::toNativeSeparators(game.absoluteFilePath()).toStdWString();
    const std::wstring directory = QDir::toNativeSeparators(game.absolutePath()).toStdWString();
    std::wstring arguments = L"\"" + path + L"\" gf " + std::to_wstring(gameLanguage);
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    const DWORD flags = CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW;
    if (!CreateProcessW(path.c_str(), arguments.data(), nullptr, nullptr, FALSE, flags, environment.data(),
                        directory.c_str(), &startup, &process)) {
        qWarning() << "Error creating game process:" << GetLastError();
        return false;
    }

    const DWORD pid = process.dwProcessId;

    QString dllPath = QDir::currentPath() + "/GflessDLL.dll";
    if (!injectDll(pid, dllPath))
        qDebug() << "Dll injection failed";
    else
        qDebug() << "Dll injected successfully";

    gameProcess.reset(process.hProcess);
    launchCode = code;
    this->displayName = displayName;

    if (ResumeThread(process.hThread) == DWORD(-1)) {
        qWarning() << "Error resuming game process:" << GetLastError();
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hThread);
        gameProcess.reset();
        launchCode.clear();
        return false;
    }

    CloseHandle(process.hThread);
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

bool GflessClient::isGameConnection(QLocalSocket *socket) const
{
    if (!gameProcess || WaitForSingleObject(gameProcess.get(), 0) != WAIT_TIMEOUT)
        return false;

    ULONG clientPid = 0;
    const auto pipe = reinterpret_cast<HANDLE>(socket->socketDescriptor());
    return GetNamedPipeClientProcessId(pipe, &clientPid) && clientPid == GetProcessId(gameProcess.get());
}

void GflessClient::handleNewConnection()
{
    while (auto *socket = gfServer->nextPendingConnection()) {
        if (!isGameConnection(socket)) {
            socket->abort();
            socket->deleteLater();
            continue;
        }

        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] { handlePipe(socket); });
        if (socket->bytesAvailable() > 0)
            handlePipe(socket);
    }
}

void GflessClient::handlePipe(QLocalSocket *socket)
{
    if (!isGameConnection(socket)) {
        socket->abort();
        return;
    }

    QByteArray output;
    const QJsonObject json = QJsonDocument::fromJson(socket->readAll()).object();
    const QString method = json["method"].toString();

    if (method == "ClientLibrary.isClientRunning")
        output = prepareResponse(json, "true");

    else if  (method == "ClientLibrary.initSession")
        output = prepareResponse(json, json["params"].toObject()["sessionId"].toString().toUtf8());

    else if (method == "ClientLibrary.queryAuthorizationCode")
        output = prepareResponse(json, std::exchange(launchCode, QString()));

    else if (method == "ClientLibrary.queryGameAccountName")
        output = prepareResponse(json, displayName);

    else
        return;

    socket->write(output);
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
