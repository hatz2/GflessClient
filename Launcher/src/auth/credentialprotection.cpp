#include "credentialprotection.h"
#include <Windows.h>
#include <dpapi.h>

QByteArray CredentialProtection::protect(const QString &credential)
{
    QByteArray bytes = credential.toUtf8();
    if (bytes.isEmpty() || bytes.size() > MAXDWORD)
        return {};

    DATA_BLOB input = {static_cast<DWORD>(bytes.size()), reinterpret_cast<BYTE*>(bytes.data())};
    DATA_BLOB output = {};
    const BOOL ok = CryptProtectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output);
    SecureZeroMemory(bytes.data(), bytes.size());
    if (!ok)
        return {};

    const QByteArray encrypted(reinterpret_cast<const char*>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return encrypted;
}

QString CredentialProtection::unprotect(const QByteArray &encryptedCredential)
{
    if (encryptedCredential.isEmpty() || encryptedCredential.size() > MAXDWORD)
        return {};

    const auto *data = reinterpret_cast<const BYTE*>(encryptedCredential.constData());
    DATA_BLOB input = {static_cast<DWORD>(encryptedCredential.size()), const_cast<BYTE*>(data)};
    DATA_BLOB output = {};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return {};

    const QString credential = QString::fromUtf8(reinterpret_cast<const char*>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    return credential;
}
