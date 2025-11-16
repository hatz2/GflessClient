#ifndef GAMEFORGEACCOUNT_H
#define GAMEFORGEACCOUNT_H

#include <QObject>
#include <QMap>
#include "nostaleauth.h"
#include "metin2auth.h"

enum class GameType {
    NosTale,
    Metin2
};

class GameforgeAccount : public QObject
{
    Q_OBJECT
public:
    explicit GameforgeAccount(
        const QString& gfEmail,
        const QString& gfPassword,
        const QString& identPath,
        const QString& installationId,
        const QString& customGamePath,
        GameType gameType,
        bool proxy,
        const QString& proxyHost,
        const QString& proxyPort,
        const QString& proxyUsername,
        const QString& proxyPassword,
        QObject *parent = nullptr
    );

    bool authenticate(bool& captcha, QString& gfChallengeId, bool& wrongCredentials);

    bool createGameAccount(const QString& name, const QString& gfLang, QJsonObject& response);

    void setToken(const QString& token);

    const QMap<QString, QString>& getGameAccounts() const;

    void updateGameAccounts();

    QString getToken(const QString& accountId) const;
    QString getEmail() const;

    QString getPassword() const;

    QString getIdentityPath() const;

    const NostaleAuth *getAuth() const;
    const Metin2Auth *getMetin2Auth() const;

    SyncNetworAccesskManager *getNetworkManager() const;

    QString getcustomClientPath() const;

    GameType getGameType() const;

private:
    QString email;
    QString password;
    QString identityPath;
    QString customClientPath;
    GameType gameType;
    NostaleAuth* nostaleAuth;
    Metin2Auth* metin2Auth;
    QMap<QString /* id */, QString /* name */> gameAccounts;
};

#endif // GAMEFORGEACCOUNT_H
