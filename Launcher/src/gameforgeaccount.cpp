#include "gameforgeaccount.h"
#include "gameaccount.h"

GameforgeAccount::GameforgeAccount(const QString &gfEmail, const QString &gfPassword, const QString& identPath, QObject *parent)
    : QObject{parent}
    , email(gfEmail)
    , password(gfPassword)
    , identityPath(identPath)
{
    auth = new NostaleAuth(std::make_shared<Identity>(identityPath), this);
}

bool GameforgeAccount::authenticate(bool &captcha, QString &gfChallengeId, bool &wrongCredentials)
{
    bool res = auth->authenticate(email, password, captcha, gfChallengeId, wrongCredentials);

    if (res) {
        gameAccounts = auth->getAccounts();
    }

    return res;
}

const QMap<QString, QString> &GameforgeAccount::getGameAccounts() const
{
    return gameAccounts;
}

QString GameforgeAccount::getToken(const QString &accountId) const
{
    return auth->getToken(accountId);
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
