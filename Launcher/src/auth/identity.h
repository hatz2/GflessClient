#ifndef IDENTITY_H
#define IDENTITY_H

#include "fingerprint.h"
#include <QFile>

class Identity
{
public:
    Identity() {}
    Identity(const QString& filePath, const QString& proxyIp, const QString& proxyPort, const QString& proxyUsername, const QString& proxyPassword, const bool useProxy);
    ~Identity();

    void update();

    Fingerprint getFingerprint() const;

    void setRequest(const QJsonValue& request);

private:
    QString filename;
    Fingerprint fingerprint;


    void initFingerprint(const QString& proxyIp, const QString& proxyPort, const QString& proxyUsername, const QString& proxyPassword, const bool useProxy);

    void save() const;
};

#endif // IDENTITY_H
