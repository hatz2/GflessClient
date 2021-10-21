#include "AccountProfile.h"

AccountProfile::AccountProfile(QString profName, QString gfAccount)
    : profileName(profName)
    , gameforgeAccount(gfAccount)
{

}

AccountProfile::AccountProfile()
{

}

const QString &AccountProfile::getGameforgeAccount() const
{
    return gameforgeAccount;
}

const QString &AccountProfile::getProfileName() const
{
    return profileName;
}

void AccountProfile::addAccount(const QString &fakeName, const QString &realName, const QString &id)
{
    QPair<QString, QString> pair(realName, id);

    accounts.insert(fakeName, pair);
}

void AccountProfile::eraseAccount(const QString &fakeName)
{
    accounts.remove(fakeName);
}

QString AccountProfile::getRealName(const QString &fakeName) const
{
    return accounts.value(fakeName).first;
}

QString AccountProfile::getId(const QString &fakeName) const
{
    return accounts.value(fakeName).second;
}

const QMap<QString, QPair<QString, QString> > &AccountProfile::getAccounts() const
{
    return accounts;
}
