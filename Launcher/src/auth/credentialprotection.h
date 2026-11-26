#ifndef CREDENTIALPROTECTION_H
#define CREDENTIALPROTECTION_H

#include <QByteArray>
#include <QString>

namespace CredentialProtection {
QByteArray protect(const QString &credential);
QString unprotect(const QByteArray &encryptedCredential);
}

#endif // CREDENTIALPROTECTION_H
