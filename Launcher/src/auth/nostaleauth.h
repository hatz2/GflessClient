#ifndef NOSTALEAUTH_H
#define NOSTALEAUTH_H

#include "syncnetworkaccessmanager.h"
#include "blackbox.h"
#include "identity.h"

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
    explicit NostaleAuth(const std::shared_ptr<Identity>& id, QObject *parent = nullptr);

    QMap<QString /* displayName */, QString /* id */> getAccounts();

    bool authenticate(const QString& email, const QString& password, bool &captcha, QString &gfChallengeId, bool& wrongCredentials);

    QString getToken(const QString& accountId);

signals:
    void captchaStart();
    void captchaEnd();

private:
    QChar getFirstNumber(QString uuid);

    QByteArray generateSecondTypeUserAgentMagic();

    QByteArray generateThirdTypeUserAgentMagic(QString accountId);

    bool sendStartTime();

    bool sendIovation(const QString& accountId);

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
};

#endif // NOSTALEAUTH_H
