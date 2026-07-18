#ifndef IDENTITY_H
#define IDENTITY_H

#include "fingerprint.h"
#include <QFile>

class Identity
{
public:
    Identity() {}
    Identity(const QString& filePath, const QString& proxyIp, const QString& proxyPort, const QString& proxyUsername, const QString& proxyPassword, const bool useProxy);

    void loadFromDisk();
    void update();

    Fingerprint getFingerprint() const;

    void setRequest(const QJsonValue& request);

private:
    QString filename;
    Fingerprint fingerprint;
    QString proxyIp;
    QString proxyPort;
    QString proxyUsername;
    QString proxyPassword;
    bool useProxy;

    void initFingerprint(const QString& proxyIp, const QString& proxyPort, const QString& proxyUsername, const QString& proxyPassword, const bool useProxy);

    void save() const;
};

#endif // IDENTITY_H
