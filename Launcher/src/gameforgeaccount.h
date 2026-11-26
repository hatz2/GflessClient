#ifndef GAMEFORGEACCOUNT_H
#define GAMEFORGEACCOUNT_H

#include <QObject>
#include <QMap>
#include "nostaleauth.h"

class GameforgeAccount : public QObject
{
    Q_OBJECT
public:
    explicit GameforgeAccount(const QString &email, const QString &identityPath, const QString &installationId,
                              const QString &customClientPath, bool useProxy, const QString &proxyIp,
                              const QString &socksPort, const QString &proxyUsername, QObject *parent = nullptr);

    bool authenticate(const QString &password, bool &captcha, QString &challengeId, bool &badCredentials);

    bool createGameAccount(const QString& name, const QString& gfLang, QJsonObject& response);

    void setSessionToken(const QString &token);
    const QString &getSessionToken() const;
    bool hasSession() const;
    void clearSession(bool keepProxyPassword = false);

    void setRememberSession(bool remember);
    bool getRememberSession() const;

    bool needsProxyPassword() const;
    void setProxyPassword(const QString &password);

    const QMap<QString, QString>& getGameAccounts() const;

    bool updateGameAccounts();

    QString getToken(const QString &accountId) const;
    QString getEmail() const;

    QString getIdentityPath() const;

    const NostaleAuth *getAuth() const;

    QString getcustomClientPath() const;

    void setOtpCode(const QString& otpCode);

    bool getOtpRequired() const;

    void refreshIdentity();

private:
    QString email;
    QString identityPath;
    QString customClientPath;
    NostaleAuth* auth;
    bool rememberSession = false;
    QMap<QString /* id */, QString /* name */> gameAccounts;
};

#endif // GAMEFORGEACCOUNT_H
