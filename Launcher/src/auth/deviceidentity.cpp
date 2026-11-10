#include "deviceidentity.h"
#include "blackbox.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

QString DeviceIdentity::directory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/identities";
    QDir().mkpath(path);

    return path;
}

QString DeviceIdentity::devicePath()
{
    return directory() + "/device.json";
}

bool DeviceIdentity::exists()
{
    return QFileInfo::exists(devicePath());
}

QDateTime DeviceIdentity::capturedAt()
{
    QFileInfo info(devicePath());

    if (!info.exists())
        return QDateTime();

    return info.lastModified();
}

bool DeviceIdentity::saveFromBlackbox(const QString &blackbox, QString &error)
{
    QByteArray identity = BlackBox::decode(blackbox.trimmed().toLocal8Bit());

    if (identity.isEmpty()) {
        error = "the blackbox could not be decoded";
        return false;
    }

    QFile file(devicePath());

    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        error = "could not write " + QDir::toNativeSeparators(file.fileName()) + ": " + file.errorString();
        return false;
    }

    if (file.write(identity) != identity.size()) {
        error = "could not write " + QDir::toNativeSeparators(file.fileName()) + ": " + file.errorString();
        file.close();
        file.remove();
        return false;
    }

    file.close();

    return true;
}

QString DeviceIdentity::createForAccount(const QString &email, QString &error)
{
    static const QRegularExpression unsafeChars("[^A-Za-z0-9._-]");

    QString fileName = QString(email).replace(unsafeChars, "_");
    QString path = directory() + "/" + fileName + ".json";

    // An account that is removed and added again keeps its own identity.
    if (QFileInfo::exists(path))
        return path;

    if (!exists()) {
        error = "there is no device identity on this PC yet";
        return QString();
    }

    if (!QFile::copy(devicePath(), path)) {
        error = "could not copy the device identity to " + QDir::toNativeSeparators(path);
        return QString();
    }

    return path;
}
