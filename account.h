#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "profile.h"
#include "nostaleauth.h"
#include <QObject>
#include <QList>

class Account : public QObject
{
    Q_OBJECT
public:
    explicit Account(QString gameforgeAccountUsername, QObject *parent = nullptr);

    void addRawAccount(const QString& displayName, const QString& id);

    void addProfile(Profile* profile);

    void removeProfile(int i);

    const QString &getGameforgeAccountUsername() const;

    const QList<Profile *> &getProfiles() const;

    const QMap<QString, QString> &getAccounts() const;

    void setAuthenticator(NostaleAuth *newAuthenticator);

    NostaleAuth *getAuthenticator() const;

    void setGfVersion(QString ver);

private:
    QString gameforgeAccountUsername;
    NostaleAuth* authenticator;
    QMap<QString /* display name */, QString /* id */> accounts; // Raw accounts
    QList<Profile*> profiles;
};

#endif // ACCOUNT_H
