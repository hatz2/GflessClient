#include "gameupdater.h"
#include <QDebug>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QCryptographicHash>
#include <QMessageBox>

GameUpdater::GameUpdater(const QVector<GameforgeAccount *> &gfAccs, const QString &gameDirPath, QObject *parent)
    : QObject{parent}
    , nostaleDirectoryPath(gameDirPath)
    , gfAccounts(gfAccs)
{
    networkManager = new SyncNetworAccesskManager(this);
}

void GameUpdater::updateFiles() const
{
    QJsonArray filesInfo = getRemoteFilesInformation();

    for (int i = 0; i < filesInfo.size(); ++i) {

        QJsonObject fileData = filesInfo[i].toObject();

        if (fileData["folder"].toBool()) {
            continue;
        }

        QString filePath = getFilePath(fileData["file"].toString());
        QByteArray localHash = getLocalHash(filePath);
        QByteArray remoteHash = fileData["sha1"].toVariant().toByteArray();

        if (localHash != remoteHash) {
            QByteArray fileContent = downloadFile(fileData["path"].toString());

            if (!saveFile(fileContent, filePath)) {
                QMessageBox::critical(nullptr, "Error", "Couldn't save file: " + filePath);
            }

            // Update custom clients
            if (fileData["file"].toString() == "NostaleClientX.exe") {
                for (const auto& gfAcc : gfAccounts) {
                    if (gfAcc->getcustomClientPath().isEmpty())
                        continue;

                    if (!saveFile(fileContent, gfAcc->getcustomClientPath())) {
                        QMessageBox::critical(nullptr, "Error", "Couldn't save file: " + gfAcc->getcustomClientPath());
                    }
                }
            }
        }

        emit setProgress((i * 100) / filesInfo.size());
    }
}

QJsonArray GameUpdater::getRemoteFilesInformation() const
{
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/patching/download/latest/nostale/default?locale=en&architecture=x64&branchToken"));
    QNetworkReply* reply;

    request.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, "application/x-www-form-urlencoded");

    reply = networkManager->get(request);

    QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();

    return response["entries"].toArray();
}

QByteArray GameUpdater::downloadFile(const QString &remotePath) const
{
    QNetworkRequest request(QUrl("http://patches.gameforge.com" + remotePath));
    QNetworkReply* reply;

    reply = networkManager->get(request);

    return reply->readAll();
}

bool GameUpdater::saveFile(const QByteArray &fileContent, const QString &filePath) const
{
    QFile file(filePath);

    if (!file.open(QFile::WriteOnly))
        return false;

    file.write(fileContent);
    file.close();

    return true;
}

QByteArray GameUpdater::getLocalHash(const QString &filePath) const
{
    QFile file(filePath);

    if (!file.open(QFile::ReadOnly)) {
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha1);

    if (!hash.addData(&file)) {
        file.close();
        return {};
    }

    file.close();
    return hash.result().toHex();
}

QString GameUpdater::getFilePath(const QString &relativePath) const
{
    return nostaleDirectoryPath + "/" + relativePath;
}
