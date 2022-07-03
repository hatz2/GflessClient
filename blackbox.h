#ifndef BLACKBOX_H
#define BLACKBOX_H

#include "fingerprint.h"
#include "identity.h"
#include <memory>

class BlackBox
{
public:
    BlackBox(const std::shared_ptr<Identity>& ident, const QJsonValue& req);

    QString encoded() const;

    static QByteArray decode(const QByteArray& blackbox);

protected:
    static const QStringList BLACKBOX_FIELDS;
    std::shared_ptr<Identity> identity;

    QByteArray encode(const QJsonObject& fingerprint) const;
};

class EncryptedBlackBox : public BlackBox
{
public:
    EncryptedBlackBox(const std::shared_ptr<Identity>& ident, const QString& accId, const QString& gsid, const QString& installationId);

    QString encrypted() const;

private:
    QString accountId;
    QString gsid;

    QByteArray encrypt(const QByteArray &str, const QByteArray &key) const;

    QJsonObject createRequest(const QString& gsid, const QString& installationId) const;
};

#endif // BLACKBOX_H
