#include "identity.h"

Identity::Identity(const QString &filePath)
    : filename(filePath)
{
    initFingerprint();
}

Identity::~Identity()
{
    save();
}

void Identity::update()
{
    fingerprint.updateTimings();
    fingerprint.updateCreation();
    fingerprint.updateServerTime();
    fingerprint.updateVector();
}

Fingerprint Identity::getFingerprint() const
{
    return fingerprint;
}

void Identity::setRequest(const QJsonValue &request)
{
    fingerprint.setRequest(request);
}

void Identity::initFingerprint()
{
    QFile file(filename);

    if (file.open(QFile::ReadOnly))
    {
        QByteArray content = file.readAll();
        fingerprint = Fingerprint(QJsonDocument::fromJson(content).object());
        file.close();
    }
}

void Identity::save() const
{
    QFile file(filename);

    if (file.open(QFile::WriteOnly))
    {
        QTextStream os(&file);

        os << fingerprint.toString();
        file.close();
    }
}
