#ifndef METIN2AUTH_H
#define METIN2AUTH_H

#include "identity.h"
#include "syncnetworkaccessmanager.h"

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

class Metin2Auth : public QObject
{
    Q_OBJECT
public:
    explicit Metin2Auth(
        const QString& identityPath,
        const QString& installationID,
        bool proxy,
        const QString& proxyHost,
        const QString& proxyPort,
        const QString& proxyUser,
        const QString& proxyPasswd,
        QObject *parent = nullptr
    );

    QMap<QString /* displayName */, QString /* id */> getAccounts();

    bool authenticate(const QString& email, const QString& password, bool &captcha, QString &gfChallengeId, bool& wrongCredentials);

    QString getToken(const QString& accountId);

    QString getProxyIp() const;
    QString getSocksPort() const;
    bool getUseProxy() const;

    QString getProxyUsername() const;

    QString getProxyPassword() const;

    SyncNetworAccesskManager *getNetworkManager() const;

    bool createGameAccount(const QString& email, const QString& name, const QString& gfLang, QJsonObject& response) const;

    QString getInstallationId() const;

    void setToken(const QString &newToken);
    QString getToken() const;

signals:
    void captchaStart();
    void captchaEnd();

private:
    QChar getFirstNumber(QString uuid);

    QByteArray generateSecondTypeUserAgentMagic();

    QByteArray generateThirdTypeUserAgentMagic(QString accountId);

    bool sendStartTime();

    bool sendSystemInformation();

    bool sendIovation(const QString& accountId);

    bool sendIovationOptions();

    void initInstallationId();

    void initCert();

    void initAllCerts();

    void initPrivateKey();

    void initGfVersion();

    QString locale;
    QString installationId;
    QString chromeVersion;
    QString gameforgeVersion;
    QString cert;
    QSslKey privateKey;
    QString token;
    QString browserUserAgent;
    QList<QSslCertificate> allCerts;
    std::shared_ptr<Identity> identity;

    SyncNetworAccesskManager* networkManager;

    QString proxyIp;
    QString socksPort;
    QString proxyUsername;
    QString proxyPassword;
    bool useProxy;
};

#endif // METIN2AUTH_H
