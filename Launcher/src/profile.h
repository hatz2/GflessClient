#ifndef PROFILE_H
#define PROFILE_H

#include <QObject>
#include <QMap>
#include <QVector>
#include "gameaccount.h"

class Profile : public QObject
{
    Q_OBJECT
public:
    explicit Profile(QString profileName, QObject *parent = nullptr);

    bool addAccount(const GameAccount& acc);

    void removeAccount(const int index);

    const QString &getProfileName() const;

    const QMap<QString, GameAccount>& getAccountsMap() const;

    const QVector<GameAccount>& getAccounts() const;

    void editAccount(const int index, const GameAccount& newAccount);

    void moveAccountUp(int index);

    void moveAccountDown(int index);

private:
    QString profileName;
    QMap<QString, GameAccount> accountsMap;
    QVector<GameAccount> accounts;
};

#endif // PROFILE_H
