#ifndef GAMEUPDATER_H
#define GAMEUPDATER_H

#include <QObject>
#include <QJsonArray>
#include "syncnetworkaccessmanager.h"
#include "gameforgeaccount.h"

class GameUpdater : public QObject
{
    Q_OBJECT
public:
    explicit GameUpdater(const QVector<GameforgeAccount*>& gfAccs, const QString& gameDirPath, QObject *parent = nullptr);

    void updateFiles() const;

signals:
    void setProgress(int value) const;

private:
    SyncNetworAccesskManager* networkManager;
    QString nostaleDirectoryPath;
    QVector<GameforgeAccount*> gfAccounts;

    QJsonArray getRemoteFilesInformation() const;

    QByteArray downloadFile(const QString& remotePath) const;

    bool saveFile(const QByteArray& fileContent, const QString& filePath) const;

    QByteArray getLocalHash(const QString& filePath) const;

    QString getFilePath(const QString& relativePath) const;
};

#endif // GAMEUPDATER_H
