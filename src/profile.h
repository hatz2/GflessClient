#ifndef PROFILE_H
#define PROFILE_H

#include <QObject>
#include <QMap>
#include <QPair>

class Profile : public QObject
{
    Q_OBJECT
public:
    explicit Profile(QString profileName, QObject *parent = nullptr);

    void addAccount(const QString& fakeName, const QString& realName, const QString& id);

    void removeAccount(const QString& fakeName);

    QString getRealName(const QString& fakeName);

    QString getId(const QString& fakeName);

    const QString &getProfileName() const;

    const QMap<QString, QPair<QString, QString>> &getAccounts() const;

private:
    QString profileName;
    QMap<QString /* fake name */, QPair<QString /* real name */, QString /* id */>> accounts;
};

#endif // PROFILE_H
