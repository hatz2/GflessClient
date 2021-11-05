#include "profile.h"

Profile::Profile(QString profileName, QObject *parent)
    : QObject(parent)
    , profileName(profileName)
{

}

void Profile::addAccount(const QString &fakeName, const QString &realName, const QString &id)
{
    accounts.insert(fakeName, QPair<QString, QString>(realName, id));
}

void Profile::removeAccount(const QString &fakeName)
{
    accounts.remove(fakeName);
}

QString Profile::getRealName(const QString &fakeName)
{
    return accounts.value(fakeName).first;
}

QString Profile::getId(const QString &fakeName)
{
    return accounts.value(fakeName).second;
}

const QString &Profile::getProfileName() const
{
    return profileName;
}

const QMap<QString, QPair<QString, QString> > &Profile::getAccounts() const
{
    return accounts;
}
