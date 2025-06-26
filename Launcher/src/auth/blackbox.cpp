#include "blackbox.h"

const QStringList BlackBox::BLACKBOX_FIELDS = {"v", "tz", "osType", "app", "vendor", "mem", "con", "lang", "plugins", "gpu", "fonts", "audioC", "width", "height", "video", "audio", "media", "permissions", "audioFP", "webglFP", "canvasFP", "creation", "uuid", "d", "osVersion", "vector", "userAgent", "serverTimeInMS", "request"};

BlackBox::BlackBox(const std::shared_ptr<Identity> &ident, const QJsonValue &req)
    : identity(ident)
{
    identity->setRequest(req);
}

QByteArray BlackBox::encode(const QJsonObject &fingerprint) const
{
    QJsonArray fingerprintArray;

    for (const auto& field : BLACKBOX_FIELDS)
        fingerprintArray.push_back(fingerprint[field]);

    QByteArray fingerprintArrayStr = QJsonDocument(fingerprintArray).toJson(QJsonDocument::JsonFormat::Compact);

    return BlackBox::encode(fingerprintArrayStr);
}

QByteArray BlackBox::decode(const QByteArray &blackbox)
{
    QByteArray decodedBlackbox = blackbox;
    decodedBlackbox = decodedBlackbox.replace("tra:", "").replace('_', '/').replace('-', '+');
    decodedBlackbox = QByteArray::fromBase64(decodedBlackbox);

    QByteArray uriDecoded;
    uriDecoded.push_back(decodedBlackbox[0]);

    for (int i = 1; i < decodedBlackbox.size(); ++i)
    {
        const char b = decodedBlackbox[i - 1];
        const char a = decodedBlackbox[i];
        const char c = a - b;

        uriDecoded.push_back(c);
    }

    QByteArray fingerprintStr = QUrl::fromPercentEncoding(uriDecoded).toLocal8Bit();
    QJsonArray fingerprintArray = QJsonDocument::fromJson(fingerprintStr).array();
    QJsonObject fingerprint;

    if (fingerprintArray.size() != BLACKBOX_FIELDS.size())
    {
        qDebug() << "BlackBox::decode Error size doesn't match";
        return QByteArray();
    }

    for (int i = 0; i < BLACKBOX_FIELDS.size(); ++i)
        fingerprint[BLACKBOX_FIELDS[i]] = fingerprintArray[i];

    return QJsonDocument(fingerprint).toJson();
}

QByteArray BlackBox::encode(const QByteArray &fingerprintArrayStr)
{
    QByteArray uriEncoded = QUrl::toPercentEncoding(fingerprintArrayStr, "-_!~*.'()");
    QByteArray blackbox;

    blackbox.push_back(uriEncoded.at(0));

    for (int i = 1; i < uriEncoded.size(); ++i)
    {
        const char a = blackbox.at(i - 1);
        const char b = uriEncoded.at(i);
        const char c = a + b;

        blackbox.push_back(c);
    }

    blackbox = blackbox.toBase64();
    blackbox = blackbox.replace('/', '_').replace('+', '-').replace('=', "");
    return "tra:" + blackbox;
}

QString BlackBox::encoded() const
{
    return QString(encode(identity->getFingerprint().json()));
}

EncryptedBlackBox::EncryptedBlackBox(const std::shared_ptr<Identity> &ident, const QString &accId, const QString &gsid, const QString &installationId)
    : BlackBox(ident, createRequest(gsid, installationId))
    , accountId(accId)
    , gsid(gsid)
{

}

QString EncryptedBlackBox::encrypted() const
{
    QByteArray blackbox;
    QByteArray key = gsid.toLatin1() + "-" + accountId.toLatin1();

    key = QCryptographicHash::hash(key, QCryptographicHash::Sha512).toHex();
    blackbox = encode(identity->getFingerprint().json());
    blackbox = encrypt(blackbox, key);
    blackbox = blackbox.toBase64();

    return blackbox;
}

QByteArray EncryptedBlackBox::encrypt(const QByteArray &str, const QByteArray &key) const
{
    QByteArray res;

    for (int i = 0; i < str.size(); ++i)
    {
        int key_index = i % key.size();
        res.push_back(str[i] ^ key[key_index] ^ key[key.size() - key_index - 1]);
    }

    return res;
}

QJsonObject EncryptedBlackBox::createRequest(const QString &gsid, const QString &installationId) const
{
    QJsonObject request;
    QJsonArray featuresArray;

    featuresArray.push_back(QRandomGenerator64::global()->bounded(1, MAXINT32));
    request["features"] = featuresArray;
    request["installation"] = installationId;
    request["session"] = gsid.left(gsid.lastIndexOf("-"));

    return request;
}
