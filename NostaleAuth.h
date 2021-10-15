#ifndef NOSTALEAUTH_H
#define NOSTALEAUTH_H

#include "SyncNetworkAccessManager.h"

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QCryptographicHash>
#include <QFile>
#include <QMap>
#include <QUuid>
#include <QRandomGenerator>
#include <QSslKey>

class NostaleAuth : public QObject
{
    Q_OBJECT
public:
    explicit NostaleAuth(QObject *parent = nullptr);

    QMap<QString /* displayName */, QString /* id */> getAccounts();

    bool authenthicate(const QString& email, const QString& password);

    QString getToken(const QString& accountId);

private:
    QChar getFirstNumber(QString uuid);

    QByteArray generateSecondTypeUserAgentMagic();

    QByteArray generateThirdTypeUserAgentMagic(QString accountId);

    bool sendStartTime();

    void initInstallationId();

    void initCert();

    void initAllCerts();

    void initPrivateKey();

    QString locale;
    QString installationId;
    QString chromeVersion;
    QString gameforgeVersion;
    QString cert;
    QSslKey privateKey;
    QString token;
    QString browserUserAgent;
    QList<QSslCertificate> allCerts;
};

#endif // NOSTALEAUTH_H
