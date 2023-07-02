#include "profile.h"
#include <algorithm>

Profile::Profile(QString profileName, QObject *parent)
    : QObject(parent)
    , profileName(profileName)
{

}

bool Profile::addAccount(const GameAccount &acc)
{
    if (accountsMap.contains(acc.getId()))
        return false;

    accountsMap.insert(acc.getId(), acc);
    accounts.push_back(acc);
    return true;
}

void Profile::removeAccount(const int index)
{
    GameAccount acc = accounts[index];
    accountsMap.remove(acc.getId());
    accounts.remove(index);
}

const QString &Profile::getProfileName() const
{
    return profileName;
}

const QMap<QString, GameAccount> &Profile::getAccountsMap() const
{
    return accountsMap;
}

const QVector<GameAccount> &Profile::getAccounts() const
{
    return accounts;
}

void Profile::editAccount(const int index, const GameAccount &newAccount)
{
    const GameAccount& oldAccount = accounts[index];

    if (!accountsMap.contains(oldAccount.getId()))
        return;

    accounts[index] = newAccount;

    accountsMap.remove(oldAccount.getId());
    accountsMap.insert(newAccount.getId(), newAccount);
}

void Profile::moveAccountUp(int index)
{
    if (index < 1)
        return;

    accounts.swapItemsAt(index, index - 1);
}

void Profile::moveAccountDown(int index)
{
    if (index >= (accounts.size()-1))
        return;

    accounts.swapItemsAt(index, index + 1);
}

void Profile::sort()
{
    std::sort(accounts.begin(), accounts.end(), [](const GameAccount& a, const GameAccount& b) {
        return a.getName() < b.getName();
    });
}
