#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <Windows.h>
#include <QRandomGenerator>
#include <QString>
#include <QDateTime>
#include <QTimeZone>
#include <QLocale>
#include <QDebug>
#include <QCryptographicHash>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "syncnetworkaccessmanager.h"

class Fingerprint
{
public:
    Fingerprint() {};

    Fingerprint(const QJsonObject& fp);

    QJsonObject json() const;

    QString toString() const;

    void updateVector();

    void updateCreation();

    void updateServerTime();

    void updateTimings();

    void setRequest(const QJsonValue& request);

private:
    static const int VERSION = 7;
    static const int UUID_LENGTH = 27;
    static const int VECTOR_LENGTH = 100;
    static const QString SERVER_FILE_GAME1_FILE;

    QJsonObject fingerprint;

    QByteArray generateUuid() const;

    QByteArray generateVector() const;

    char randomAsciiCharacter() const;

    QByteArray randomString(int size) const;

    QString getServerDate() const;
};

#endif // FINGERPRINT_H
