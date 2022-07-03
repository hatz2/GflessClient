#include "fingerprint.h"

const QString Fingerprint::SERVER_FILE_GAME1_FILE = "https://gameforge.com/tra/game1.js";

Fingerprint::Fingerprint(const QJsonObject& fp)
    : fingerprint(fp)
{

}

QJsonObject Fingerprint::json() const
{
    return fingerprint;
}

QString Fingerprint::toString() const
{
    return QJsonDocument(fingerprint).toJson();
}

void Fingerprint::updateVector()
{
    QByteArray content = QByteArray::fromBase64(fingerprint["vector"].toString().toLatin1());
    content = content.left(content.lastIndexOf(' '));
    content = content.mid(1) + randomAsciiCharacter();

    QByteArray newVector = content + " " + QString::number(QDateTime::currentMSecsSinceEpoch()).toLocal8Bit();
    fingerprint["vector"] = QString(newVector.toBase64());
}

void Fingerprint::updateCreation()
{
    fingerprint["creation"] = QDateTime::currentDateTime().toUTC().toString(Qt::DateFormat::ISODateWithMs);
}

void Fingerprint::updateServerTime()
{
    fingerprint["serverTimeInMS"] = getServerDate();
}

void Fingerprint::updateTimings()
{
    fingerprint["dP"] = QRandomGenerator64::global()->bounded(0, 50);
    fingerprint["dF"] = QRandomGenerator64::global()->bounded(0, 50);
    fingerprint["dW"] = QRandomGenerator64::global()->bounded(0, 50);
    fingerprint["dC"] = QRandomGenerator64::global()->bounded(0, 50);
    fingerprint["d"] = fingerprint["dP"].toInt() + fingerprint["dF"].toInt() +
            fingerprint["dW"].toInt() + fingerprint["dC"].toInt() + QRandomGenerator64::global()->bounded(0, 10);
}

void Fingerprint::setRequest(const QJsonValue &request)
{
    fingerprint["request"] = request;
}

QByteArray Fingerprint::generateUuid() const
{
    QString str = randomString(UUID_LENGTH);
    return str.toLocal8Bit().toBase64().toLower();
}

QByteArray Fingerprint::generateVector() const
{
    QByteArray str = randomString(VECTOR_LENGTH);
    qint64 time = QDateTime::currentMSecsSinceEpoch();
    QByteArray vec = str + " " + QString::number(time).toLocal8Bit();

    return vec.toBase64();
}

char Fingerprint::randomAsciiCharacter() const
{
    return QRandomGenerator::global()->bounded(32, 126);
}

QByteArray Fingerprint::randomString(int size) const
{
    QByteArray str;

    for (int i = 0; i < size; ++i)
    {
        char r = randomAsciiCharacter();
        str.push_back(r);
    }

    return str;
}

QString Fingerprint::getServerDate() const
{
    QUrl url(SERVER_FILE_GAME1_FILE);
    SyncNetworAccesskManager network;
    QNetworkRequest request(url);
    QNetworkReply* reply = network.get(request);
    QByteArray date = reply->rawHeader("Date").replace("GMT", "UTC");
    QDateTime dateTime = QLocale(QLocale::English).toDateTime(date.replace("GMT", "UTC"), "ddd, d MMM yyyy HH:mm:ss t");

    reply->deleteLater();

    return dateTime.toString(Qt::DateFormat::ISODateWithMs);
}
