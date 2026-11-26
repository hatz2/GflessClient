#include "gameforgeaccount.h"

GameforgeAccount::GameforgeAccount(const QString &email, const QString &identityPath, const QString &installationId,
                                   const QString &customClientPath, bool useProxy, const QString &proxyIp,
                                   const QString &socksPort, const QString &proxyUsername, QObject *parent)
    : QObject(parent)
    , email(email)
    , identityPath(identityPath)
    , customClientPath(customClientPath)
{
    auth = new NostaleAuth(identityPath, installationId, useProxy, proxyIp, socksPort, proxyUsername, this);
}

bool GameforgeAccount::authenticate(const QString &password, bool &captcha, QString &challengeId, bool &badCredentials)
{
    return auth->authenticate(email, password, captcha, challengeId, badCredentials);
}

bool GameforgeAccount::createGameAccount(const QString &name, const QString &gfLang, QJsonObject& response)
{
    return auth->createGameAccount(email, name, gfLang, response);
}

void GameforgeAccount::setSessionToken(const QString &token)
{
    auth->setSessionToken(token);
}

const QString &GameforgeAccount::getSessionToken() const
{
    return auth->getSessionToken();
}

bool GameforgeAccount::hasSession() const
{
    return !getSessionToken().isEmpty();
}

void GameforgeAccount::clearSession(bool keepProxyPassword)
{
    setSessionToken({});
    auth->setOtpCode({});
    if (!keepProxyPassword && !auth->getProxyPassword().isEmpty())
        setProxyPassword({});
    gameAccounts.clear();
}

void GameforgeAccount::setRememberSession(bool remember)
{
    rememberSession = remember;
}

bool GameforgeAccount::getRememberSession() const
{
    return rememberSession;
}

bool GameforgeAccount::needsProxyPassword() const
{
    return auth->getUseProxy() && !auth->getProxyUsername().isEmpty() && auth->getProxyPassword().isEmpty();
}

void GameforgeAccount::setProxyPassword(const QString &password)
{
    auth->setProxyPassword(password);
}

const QMap<QString, QString> &GameforgeAccount::getGameAccounts() const
{
    return gameAccounts;
}

bool GameforgeAccount::updateGameAccounts()
{
    return auth->getAccounts(gameAccounts);
}

QString GameforgeAccount::getToken(const QString &accountId) const
{
    return auth->getToken(accountId);
}

QString GameforgeAccount::getEmail() const
{
    return email;
}

QString GameforgeAccount::getIdentityPath() const
{
    return identityPath;
}

const NostaleAuth *GameforgeAccount::getAuth() const
{
    return auth;
}

void GameforgeAccount::setOtpCode(const QString &otpCode)
{
    auth->setOtpCode(otpCode);
}

bool GameforgeAccount::getOtpRequired() const
{
    return auth->getOtpRequired();
}

void GameforgeAccount::refreshIdentity()
{
    auth->refreshIdentity();
}

QString GameforgeAccount::getcustomClientPath() const
{
    return customClientPath;
}
