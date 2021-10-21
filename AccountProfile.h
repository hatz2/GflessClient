#ifndef ACCOUNTPROFILE_H
#define ACCOUNTPROFILE_H

#include <QObject>
#include <QMap>
#include <QPair>

class AccountProfile
{
public:
    AccountProfile(QString profName, QString gfAccount);

    AccountProfile();

    const QString &getGameforgeAccount() const;

    const QString &getProfileName() const;

    void addAccount(const QString& fakeName, const QString& realName, const QString& id);

    void eraseAccount(const QString& fakeName);

    QString getRealName(const QString& fakeName) const;

    QString getId(const QString& fakeName) const;

    const QMap<QString, QPair<QString, QString> > &getAccounts() const;

private:
    QString profileName;
    QString gameforgeAccount;
    QMap<QString /* fake name */, QPair<QString /* real name */, QString /* id */>> accounts;

};

#endif // ACCOUNTPROFILE_H
