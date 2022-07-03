#include "nostaleauth.h"

NostaleAuth::NostaleAuth(const std::shared_ptr<Identity> &id, QObject *parent) : QObject(parent), identity(id)
{
    this->locale = "en_GB";
    this->chromeVersion = "C2.2.23.1813";
    this->gameforgeVersion = "2.2.23";
    this->browserUserAgent = "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36";

    initCert();
    initInstallationId();
    initAllCerts();
    initPrivateKey();
}

QMap<QString, QString> NostaleAuth::getAccounts()
{
    QJsonObject jsonResponse;
    QMap<QString, QString> accounts;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/user/accounts"));
    SyncNetworAccesskManager networkManager(this);
    QNetworkReply* reply = nullptr;

    if (token.isEmpty())
        return QMap<QString, QString>();

    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("TNT-Installation-Id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");

    reply = networkManager.get(request);

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return QMap<QString, QString>();

    jsonResponse = QJsonDocument::fromJson(reply->readAll()).object();

    for (const auto& key : jsonResponse.keys())
    {
        auto accountData = jsonResponse[key].toObject();
        accounts.insert(accountData["displayName"].toString(), accountData["id"].toString());
    }

    reply->deleteLater();

    return accounts;
}

bool NostaleAuth::authenthicate(const QString &email, const QString &password)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/auth/sessions"));
    SyncNetworAccesskManager networkManager(this);
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("TNT-Installation-Id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");

    content["email"] = email;
    content["locale"] = locale;
    content["password"] = password;

    reply = networkManager.post(request, QJsonDocument(content).toJson());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 201)
        return false;

    jsonResponse = QJsonDocument::fromJson(reply->readAll()).object();

    qDebug() << QJsonDocument(jsonResponse).toJson();

    token = jsonResponse["token"].toString();

    reply->deleteLater();

    return true;
}

QString NostaleAuth::getToken(const QString &accountId)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/auth/thin/codes"));
    SyncNetworAccesskManager networkManager;
    QNetworkReply* reply = nullptr;

    if (token.isEmpty())
        return QByteArray();

    if (!sendIovation(accountId))
        return QByteArray();

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("User-Agent", "Chrome/" + chromeVersion.toUtf8() + " (" + generateThirdTypeUserAgentMagic(accountId) + ")");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");
    request.setRawHeader("tnt-installation-id", installationId.toUtf8());

    QString gsid = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces) + "-" + QString::number(QRandomGenerator::global()->bounded(1000, 9999));
    identity->update();
    EncryptedBlackBox blackbox(identity, accountId, gsid, installationId);

    content["platformGameAccountId"] = accountId;
    content["gsid"] = gsid;
    content["blackbox"] = blackbox.encrypted();
    content["gameId"] = "dd4e22d6-00d1-44b9-8126-d8b40e0cd7c9";

    reply = networkManager.post(request, QJsonDocument(content).toJson());

    QByteArray response = reply->readAll();

    qDebug() << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 201)
        return QByteArray();

    jsonResponse = QJsonDocument::fromJson(response).object();

    reply->deleteLater();

    return jsonResponse["code"].toString();
}

QChar NostaleAuth::getFirstNumber(QString uuid)
{
    for (const auto& c : uuid)
        if (c.isDigit())
            return c;

    return QChar();
}

QByteArray NostaleAuth::generateThirdTypeUserAgentMagic(QString accountId)
{
    QByteArray hashOfCert, hashOfVersion, hashOfInstallationId, hashOfAccountId, hashOfSum;
    QChar firstLetter = getFirstNumber(installationId);
    QString firstTwoLettersOfAccountId = accountId.left(2);

    if (firstLetter == QChar::Null || firstLetter.digitValue() % 2 == 0)
    {
        hashOfCert = QCryptographicHash::hash(cert.toUtf8(), QCryptographicHash::Algorithm::Sha256).toHex();
        hashOfVersion = QCryptographicHash::hash(chromeVersion.toUtf8(), QCryptographicHash::Algorithm::Sha1).toHex();
        hashOfInstallationId = QCryptographicHash::hash(installationId.toUtf8(), QCryptographicHash::Algorithm::Sha256).toHex();
        hashOfAccountId = QCryptographicHash::hash(accountId.toUtf8(), QCryptographicHash::Algorithm::Sha1).toHex();
        hashOfSum = QCryptographicHash::hash(hashOfCert + hashOfVersion + hashOfInstallationId + hashOfAccountId, QCryptographicHash::Sha256).toHex();

        return firstTwoLettersOfAccountId.toUtf8() + hashOfSum.left(8);
    }

    else
    {
        hashOfCert = QCryptographicHash::hash(cert.toUtf8(), QCryptographicHash::Algorithm::Sha1).toHex();
        hashOfVersion = QCryptographicHash::hash(chromeVersion.toUtf8(), QCryptographicHash::Algorithm::Sha256).toHex();
        hashOfInstallationId = QCryptographicHash::hash(installationId.toUtf8(), QCryptographicHash::Algorithm::Sha1).toHex();
        hashOfAccountId = QCryptographicHash::hash(accountId.toUtf8(), QCryptographicHash::Algorithm::Sha256).toHex();
        hashOfSum = QCryptographicHash::hash(hashOfCert + hashOfVersion + hashOfInstallationId + hashOfAccountId, QCryptographicHash::Sha256).toHex();

        return firstTwoLettersOfAccountId.toUtf8() + hashOfSum.right(8);
    }
}

bool NostaleAuth::sendStartTime()
{
    QJsonObject payload, clientVersionInfo;
    QNetworkRequest request(QUrl("https://events.gameforge.com"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    SyncNetworAccesskManager networkManager(this);
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Host", "events.gameforge.com");
    request.setRawHeader("User-Agent", "GameforgeClient/" + gameforgeVersion.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");

    clientVersionInfo.insert("branch", "master");
    clientVersionInfo.insert("commit_id", "27942713");
    clientVersionInfo.insert("version", chromeVersion);

    payload["client_installation_id"] = installationId;
    payload["client_locale"] = "pol_pol";
    payload["client_session_id"] = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
    payload["client_version_info"] = clientVersionInfo;
    payload["id"] = 0;
    payload["localtime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload["start_count"] = 1;
    payload["start_time"] = QRandomGenerator::global()->bounded(1500, 10000);
    payload["type"] = "start_time";

    sslConfig.setLocalCertificate(allCerts.first());
    sslConfig.setCaCertificates(allCerts);
    sslConfig.setPrivateKey(privateKey);
    request.setSslConfiguration(sslConfig);

    reply = networkManager.post(request, QJsonDocument(payload).toJson());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    qDebug() << reply->readAll();

    reply->deleteLater();

    return true;
}

bool NostaleAuth::sendIovation(const QString& accountId)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/auth/iovation"));
    SyncNetworAccesskManager networkManager(this);
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("TNT-Installation-Id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());

    identity->update();
    BlackBox blackbox(identity, QJsonValue::Null);

    content["accountId"] = accountId;
    content["blackbox"] = blackbox.encoded();
    content["type"] = "play_now";

    reply = networkManager.post(request, QJsonDocument(content).toJson());

    reply->deleteLater();

    QByteArray response = reply->readAll();

    qDebug() << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    jsonResponse = QJsonDocument::fromJson(response).object();

    if (jsonResponse["status"] != "ok")
        return false;

    return true;
}

void NostaleAuth::initInstallationId()
{
    QSettings s("HKEY_CURRENT_USER\\SOFTWARE\\Gameforge4d\\GameforgeClient\\MainApp", QSettings::NativeFormat);

    this->installationId = s.value("InstallationId").toString();

    if (installationId.isEmpty())
    {
        qDebug() << "Couldn't find InstallationId";
        qDebug() << "Trying to find it from another registry";

        QSettings sett("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Gameforge4d\\GameforgeClient\\MainApp", QSettings::NativeFormat);

        this->installationId = sett.value("InstallationId").toString();

        if (installationId.isEmpty())
            qDebug() << "Couldn't find InstallationId";
    }
}

void NostaleAuth::initCert()
{
    QFile file(":/resources/all_certs.pem");
    QByteArray data;
    int start, end;

    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        qDebug() << "Error opening cert file " << file.errorString();
        return;
    }

    data = file.readAll();
    start = data.indexOf("-----BEGIN CERTIFICATE-----");
    end = data.indexOf("-----END CERTIFICATE-----", start);

    this->cert = data.mid(start, end - start + strlen("-----END CERTIFICATE-----") + 1);
}

void NostaleAuth::initAllCerts()
{
    allCerts = QSslCertificate::fromPath(":/resources/all_certs.pem");
}

void NostaleAuth::initPrivateKey()
{
    QFile certFile(":/resources/all_certs.pem");
    QByteArray certData, key;
    int start, end;

    if (!certFile.open(QFile::ReadOnly))
    {
        qDebug() << "Error opening cert file " << certFile.errorString();
        return;
    }

    certData = certFile.readAll();
    start = certData.indexOf("-----BEGIN PRIVATE KEY-----");
    end = certData.indexOf("-----END PRIVATE KEY-----", start);
    key =  certData.mid(start, end - start + strlen("-----END PRIVATE KEY-----") + 1);

    this->privateKey = QSslKey(key, QSsl::Rsa);
}
