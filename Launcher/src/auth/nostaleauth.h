#ifndef NOSTALEAUTH_H
#define NOSTALEAUTH_H

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

class NostaleAuth : public QObject
{
    Q_OBJECT
public:
    explicit NostaleAuth(
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

    bool getUserInformation();

    bool sendStartTime();

    bool sendSystemInformation();

    bool sendGameLaunch(const QString &accountId);

    bool sendGameStarted(const QString& accountId);

    bool sendIovation(const QString& accountId);

    bool sendIovationOptions();

    void initInstallationId();

    void initCert();

    void initAllCerts();

    void initPrivateKey();

    void initGfVersion();

    QString createBlackbox();

    QString createEncryptedBlackbox(const QString &gsid, const QString &installationId, const QString &accountId);

    QSslConfiguration getCustomSslConfig() const;

    const QString commitId = "eda2b413";
    const QString gameId = "dd4e22d6-00d1-44b9-8126-d8b40e0cd7c9";
    QString locale;
    QString installationId;
    QString chromeVersion;
    QString gameforgeVersion;
    QString cert;
    QSslKey privateKey;
    QString token;
    QString browserUserAgent;
    QString gfAccountId;
    QString email;
    QString frontendVersion;
    QString version;
    QString gameSessionId;
    QString eventsSessionId;
    QList<QSslCertificate> allCerts;
    std::shared_ptr<Identity> identity;

    SyncNetworAccesskManager* networkManager;

    QString proxyIp;
    QString socksPort;
    QString proxyUsername;
    QString proxyPassword;
    bool useProxy;
};

#endif // NOSTALEAUTH_H
