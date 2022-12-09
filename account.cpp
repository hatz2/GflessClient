#include "account.h"

Account::Account(QString gameforgeAccountUsername, QObject *parent)
    : QObject(parent)
    , gameforgeAccountUsername(gameforgeAccountUsername)
{
    authenticator = nullptr;
}

void Account::addRawAccount(const QString &displayName, const QString &id)
{
    accounts.insert(displayName, id);
}

void Account::addProfile(Profile *profile)
{
    profiles.append(profile);
}

void Account::removeProfile(int i)
{
    if (i < 0 || i >= profiles.size())
        return;

    profiles.removeAt(i);
}

const QString &Account::getGameforgeAccountUsername() const
{
    return gameforgeAccountUsername;
}

const QList<Profile *> &Account::getProfiles() const
{
    return profiles;
}

const QMap<QString, QString> &Account::getAccounts() const
{
    return accounts;
}

void Account::setAuthenticator(NostaleAuth *newAuthenticator)
{
    authenticator = newAuthenticator;
}

NostaleAuth *Account::getAuthenticator() const
{
    return authenticator;
}

void Account::setGfVersion(QString ver)
{
    authenticator->setGfVersion(ver);
}
