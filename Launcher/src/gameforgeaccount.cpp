#include "gameforgeaccount.h"

GameforgeAccount::GameforgeAccount(const QString &gfEmail,
    const QString &gfPassword,
    const QString& identPath,
    const QString &installationId,
    const QString& customGamePath,
    GameType gameType,
    bool proxy,
    const QString &proxyHost,
    const QString &proxyPort,
    const QString &proxyUsername,
    const QString &proxyPassword,
    QObject *parent
)
    : QObject{parent}
    , email(gfEmail)
    , password(gfPassword)
    , identityPath(identPath)
    , customClientPath(customGamePath)
    , gameType(gameType)
    , nostaleAuth(nullptr)
    , metin2Auth(nullptr)
{
    if (gameType == GameType::NosTale) {
        nostaleAuth = new NostaleAuth(
            identityPath,
            installationId,
            proxy,
            proxyHost,
            proxyPort,
            proxyUsername,
            proxyPassword,
            this
        );
    } else if (gameType == GameType::Metin2) {
        metin2Auth = new Metin2Auth(
            identityPath,
            installationId,
            proxy,
            proxyHost,
            proxyPort,
            proxyUsername,
            proxyPassword,
            this
        );
    }
}

bool GameforgeAccount::authenticate(bool &captcha, QString &gfChallengeId, bool &wrongCredentials)
{
    bool res = false;

    if (gameType == GameType::NosTale && nostaleAuth) {
        res = nostaleAuth->authenticate(email, password, captcha, gfChallengeId, wrongCredentials);
    } else if (gameType == GameType::Metin2 && metin2Auth) {
        res = metin2Auth->authenticate(email, password, captcha, gfChallengeId, wrongCredentials);
    }

    if (res) {
        updateGameAccounts();
    }

    return res;
}

bool GameforgeAccount::createGameAccount(const QString &name, const QString &gfLang, QJsonObject& response)
{
    if (gameType == GameType::NosTale && nostaleAuth) {
        return nostaleAuth->createGameAccount(this->email, name, gfLang, response);
    } else if (gameType == GameType::Metin2 && metin2Auth) {
        return metin2Auth->createGameAccount(this->email, name, gfLang, response);
    }

    return false;
}

void GameforgeAccount::setToken(const QString &token)
{
    if (gameType == GameType::NosTale && nostaleAuth) {
        nostaleAuth->setToken(token);
    } else if (gameType == GameType::Metin2 && metin2Auth) {
        metin2Auth->setToken(token);
    }
}

const QMap<QString, QString> &GameforgeAccount::getGameAccounts() const
{
    return gameAccounts;
}

void GameforgeAccount::updateGameAccounts()
{
    if (gameType == GameType::NosTale && nostaleAuth) {
        gameAccounts = nostaleAuth->getAccounts();
    } else if (gameType == GameType::Metin2 && metin2Auth) {
        gameAccounts = metin2Auth->getAccounts();
    }
}

QString GameforgeAccount::getToken(const QString &accountId) const
{
    if (gameType == GameType::NosTale && nostaleAuth) {
        return nostaleAuth->getToken(accountId);
    } else if (gameType == GameType::Metin2 && metin2Auth) {
        return metin2Auth->getToken(accountId);
    }

    return QString();
}

QString GameforgeAccount::getEmail() const
{
    return email;
}

QString GameforgeAccount::getPassword() const
{
    return password;
}

QString GameforgeAccount::getIdentityPath() const
{
    return identityPath;
}

const NostaleAuth *GameforgeAccount::getAuth() const
{
    return nostaleAuth;
}

const Metin2Auth *GameforgeAccount::getMetin2Auth() const
{
    return metin2Auth;
}

SyncNetworAccesskManager *GameforgeAccount::getNetworkManager() const
{
    if (gameType == GameType::NosTale && nostaleAuth) {
        return nostaleAuth->getNetworkManager();
    } else if (gameType == GameType::Metin2 && metin2Auth) {
        return metin2Auth->getNetworkManager();
    }

    return nullptr;
}

QString GameforgeAccount::getcustomClientPath() const
{
    return customClientPath;
}

GameType GameforgeAccount::getGameType() const
{
    return gameType;
}
