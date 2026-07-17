#include "identity.h"

Identity::Identity(const QString &filePath, const QString &proxyIp, const QString &proxyPort, const QString &proxyUsername, const QString &proxyPassword, const bool useProxy)
    : filename(filePath)
    , proxyIp(proxyIp)
    , proxyPort(proxyPort)
    , proxyUsername(proxyUsername)
    , proxyPassword(proxyPassword)
    , useProxy(useProxy)
{
    initFingerprint(proxyIp, proxyPort, proxyUsername, proxyPassword, useProxy);
}

void Identity::loadFromDisk()
{
    initFingerprint(proxyIp, proxyPort, proxyUsername, proxyPassword, useProxy);
}

void Identity::update()
{
    fingerprint.updateServerTime();
    fingerprint.updateVector();
    fingerprint.updateCreation();
    fingerprint.updateTimings();
    save();
}

Fingerprint Identity::getFingerprint() const
{
    return fingerprint;
}

void Identity::setRequest(const QJsonValue &request)
{
    fingerprint.setRequest(request);
}

void Identity::initFingerprint(const QString &proxyIp, const QString &proxyPort, const QString &proxyUsername, const QString &proxyPassword, const bool useProxy)
{
    QFile file(filename);

    if (file.open(QFile::ReadOnly))
    {
        QByteArray content = file.readAll();
        fingerprint = Fingerprint(QJsonDocument::fromJson(content).object(), proxyIp, proxyPort, proxyUsername, proxyPassword, useProxy);
        file.close();
    }
}

void Identity::save() const
{
    QFile file(filename);

    if (file.open(QFile::WriteOnly))
    {
        QTextStream os(&file);

        os << fingerprint.toString();
        file.close();
    }
}
