#include "metin2auth.h"
#include "blackbox.h"
#include <QNetworkProxy>
#include <QtZlib/zlib.h>

Metin2Auth::Metin2Auth(const QString &identityPath, const QString& installationID, bool proxy, const QString &proxyHost, const QString &proxyPort, const QString &proxyUser, const QString &proxyPasswd, QObject *parent)
    : QObject(parent)
    , installationId(installationID)
    , proxyIp(proxyHost)
    , socksPort(proxyPort)
    , proxyUsername(proxyUser)
    , proxyPassword(proxyPasswd)
    , useProxy(proxy)
{
    identity = std::make_shared<Identity>(identityPath, proxyHost, proxyPort, proxyUser, proxyPasswd, proxy);

    this->locale = QLocale().name();
    this->browserUserAgent = "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36";

    networkManager = new SyncNetworAccesskManager(this);

    if (useProxy) {
        QNetworkProxy proxy(
            QNetworkProxy::ProxyType::Socks5Proxy,
            proxyIp,
            socksPort.toUInt()
        );

        if (!proxyUsername.isEmpty()) {
            proxy.setUser(proxyUsername);
            proxy.setPassword(proxyPassword);
        }

        networkManager->setProxy(proxy);
    }


    initGfVersion();
    initCert();
    initInstallationId();
    initAllCerts();
    initPrivateKey();
}

// Thanks ChatGPT
QByteArray decompressGzip_Metin2(const QByteArray &data) {
    if (data.isEmpty()) {
        return QByteArray();
    }

    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
        return QByteArray();
    }

    strm.next_in = (Bytef *)data.data();
    strm.avail_in = data.size();

    int ret;
    char buffer[16384];
    QByteArray out;

    do {
        strm.next_out = reinterpret_cast<Bytef *>(buffer);
        strm.avail_out = sizeof(buffer);

        ret = inflate(&strm, Z_NO_FLUSH);
        if (out.size() < static_cast<int>(strm.total_out)) {
            out.append(buffer, strm.total_out - out.size());
        }
    } while (ret == Z_OK);

    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        return QByteArray(); // Failed to decompress
    }

    return out;
}

QMap<QString, QString> Metin2Auth::getAccounts()
{
    QJsonObject jsonResponse;
    QMap<QString, QString> accounts;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/user/accounts"));
    QNetworkReply* reply = nullptr;

    if (token.isEmpty())
        return QMap<QString, QString>();

    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("TNT-Installation-Id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");
    // request.setRawHeader("Accept", "*/*");
    // request.setRawHeader("Accept-Language", "en-US,en;q=0.9");
    // request.setRawHeader("accept-encoding", "gzip, deflate, br");

    reply = networkManager->get(request);

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return QMap<QString, QString>();

    QByteArray replyContent = reply->readAll();
    jsonResponse = QJsonDocument::fromJson(replyContent).object();
    // jsonResponse = QJsonDocument::fromJson(decompressGzip_Metin2(replyContent)).object();

    for (const auto& key : jsonResponse.keys())
    {
        auto accountData = jsonResponse[key].toObject();
        auto guls = accountData["guls"].toObject();

        if (guls["game"].toString() != "metin2")
            continue;

        accounts.insert(accountData["id"].toString(), accountData["displayName"].toString());
    }

    reply->deleteLater();

    return accounts;
}

bool Metin2Auth::authenticate(const QString &email, const QString &password, bool& captcha, QString& gfChallengeId, bool &wrongCredentials)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/auth/sessions"));
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.9");
    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("TNT-Installation-Id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("accept-encoding", "gzip, deflate, br");

    identity->update();
    BlackBox blackbox(identity, QJsonValue::Null);

    content["blackbox"] = blackbox.encoded();
    content["email"] = email;
    content["locale"] = locale;
    content["password"] = password;

    reply = networkManager->post(request, QJsonDocument(content).toJson(QJsonDocument::Compact));

    jsonResponse = QJsonDocument::fromJson(reply->readAll()).object();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 201)
    {
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409) // Conflict Captcha
        {
            gfChallengeId = reply->rawHeader("gf-challenge-id").split(';').first();
            captcha = true;
            reply->deleteLater();
            return false;
        }
        else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 403) // Incorrect username or password
        {
            wrongCredentials = true;
            reply->deleteLater();
            return false;
        }
        else
        {
            reply->deleteLater();
            return false;
        }
    }

    token = jsonResponse["token"].toString();
    reply->deleteLater();

    return true;
}

QString Metin2Auth::getToken(const QString &accountId)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/auth/thin/codes"));
    QNetworkReply* reply = nullptr;

    if (token.isEmpty())
        return QByteArray();

    // if (!sendSystemInformation())
    //     return {};

    // if (!sendStartTime())
    //     return QByteArray();

    if (!sendIovation(accountId))
        return QByteArray();

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("User-Agent", "Chrome/" + chromeVersion.toUtf8() + " (" + generateThirdTypeUserAgentMagic(accountId) + ")");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");
    request.setRawHeader("tnt-installation-id", installationId.toUtf8());
    request.setRawHeader("accept-encoding", "gzip, deflate, br");

    QString gsid = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces) + "-" + QString::number(QRandomGenerator::global()->bounded(1000, 9999));
    identity->update();
    EncryptedBlackBox blackbox(identity, accountId, gsid, installationId);

    content["platformGameAccountId"] = accountId;
    content["gsid"] = gsid;
    content["blackbox"] = blackbox.encrypted();
    content["gameId"] = "fab180a3-cd65-4b7e-bd0e-2ef77fd0c258.tr-TR";

    reply = networkManager->post(request, QJsonDocument(content).toJson());

    QByteArray response = reply->readAll();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 201)
        return QByteArray();

    jsonResponse = QJsonDocument::fromJson(response).object();

    reply->deleteLater();

    return jsonResponse["code"].toString();
}

QChar Metin2Auth::getFirstNumber(QString uuid)
{
    for (const auto& c : uuid)
        if (c.isDigit())
            return c;

    return QChar();
}

QByteArray Metin2Auth::generateThirdTypeUserAgentMagic(QString accountId)
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

bool Metin2Auth::sendStartTime()
{
    QJsonObject payload, clientVersionInfo;
    QNetworkRequest request(QUrl("https://events2.gameforge.com"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Host", "events.gameforge.com");
    request.setRawHeader("User-Agent", "GameforgeClient/" + gameforgeVersion.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");

    clientVersionInfo.insert("branch", "2-8-0");
    clientVersionInfo.insert("commit_id", "2f02b72f");
    clientVersionInfo.insert("version", chromeVersion.remove('C'));

    payload["client_installation_id"] = installationId;
    payload["client_locale"] = "gbr_eng";
    payload["client_session_id"] = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
    payload["client_version_info"] = clientVersionInfo;
    payload["id"] = 0;
    payload["localtime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload["start_count"] = 1;
    payload["start_time"] = QRandomGenerator::global()->bounded(1500, 10000);
    payload["type"] = "start_time";


    QFile f(":/resources/client.p12");

    if (f.open(QFile::ReadOnly)) {
        QSslCertificate cert;
        QSslKey key;
        QList<QSslCertificate> caCerts;

        if (QSslCertificate::importPkcs12(&f, &key, &cert, &caCerts, "Iroo1pasaife6Zax9caijiebaHuH9aeh")) {
            sslConfig.setLocalCertificate(cert);
            sslConfig.setCaCertificates(caCerts);
            sslConfig.setPrivateKey(key);
            request.setSslConfiguration(sslConfig);
        }

        f.close();
    }

    reply = networkManager->post(request, QJsonDocument(payload).toJson());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    qDebug() << reply->readAll();

    reply->deleteLater();

    return true;
}

bool Metin2Auth::sendSystemInformation()
{
    QJsonObject payload, clientVersionInfo;
    QNetworkRequest request(QUrl("https://events.gameforge.com"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Host", "events.gameforge.com");
    request.setRawHeader("User-Agent", "GameforgeClient/" + gameforgeVersion.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");

    clientVersionInfo.insert("branch", "2-8-0");
    clientVersionInfo.insert("commit_id", "2f02b72f");
    clientVersionInfo.insert("version", chromeVersion.remove('C'));

    payload["client_installation_id"] = installationId;
    payload["client_locale"] = "gbr_eng";
    payload["client_session_id"] = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
    payload["client_version_info"] = clientVersionInfo;
    payload["id"] = 0;
    payload["localtime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload["type"] = "system_information";
    payload["version"] = "10.0.22621";
    payload["system_locale"] = "en-en";
    payload["ram"] = 12341234;
    payload["os_architecture"] = "x64";
    payload["frontendVersion"] = "0.468.0";


    QFile f(":/resources/client.p12");

    if (f.open(QFile::ReadOnly)) {
        QSslCertificate cert;
        QSslKey key;
        QList<QSslCertificate> caCerts;

        if (QSslCertificate::importPkcs12(&f, &key, &cert, &caCerts, "Iroo1pasaife6Zax9caijiebaHuH9aeh")) {
            sslConfig.setLocalCertificate(cert);
            sslConfig.setCaCertificates(caCerts);
            sslConfig.setPrivateKey(key);
            request.setSslConfiguration(sslConfig);
        }

        f.close();
    }

    reply = networkManager->post(request, QJsonDocument(payload).toJson());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    qDebug() << reply->readAll();

    reply->deleteLater();

    return true;
}

bool Metin2Auth::sendIovation(const QString& accountId)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/auth/iovation"));
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Accept-Encoding", "gzip, deflate, br");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.9");
    request.setRawHeader("User-Agent", browserUserAgent.toLocal8Bit());
    request.setRawHeader("TNT-Installation-Id", installationId.toLocal8Bit());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Authorization", "Bearer " + token.toLocal8Bit());

    identity->update();
    BlackBox blackbox(identity, QJsonValue::Null);

    content["accountId"] = accountId;
    content["blackbox"] = blackbox.encoded();
    content["type"] = "play_now";

    if (!sendIovationOptions())
        return false;

    QByteArray body = QJsonDocument(content).toJson(QJsonDocument::JsonFormat::Compact);
    reply = networkManager->post(request, body);

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

bool Metin2Auth::sendIovationOptions()
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/auth/iovation"));
    QNetworkReply* reply = nullptr;

    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("accept-encoding", "gzip, deflate, br");
    request.setRawHeader("accept-language", "en-US,en;q=0.9");
    request.setRawHeader("access-control-request-headers", "authorization,content-type,tnt-installation-id");
    request.setRawHeader("access-control-request-method", "POST");
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("User-Agent", browserUserAgent.toLocal8Bit());


    reply = networkManager->sendCustomRequest(request, "OPTIONS");

    QByteArray response = reply->readAll();

    qDebug() << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 204)
        return false;

    jsonResponse = QJsonDocument::fromJson(response).object();

    return true;
}

void Metin2Auth::initInstallationId()
{
    if (!installationId.isEmpty()) {
        return;
    }

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

void Metin2Auth::initCert()
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

void Metin2Auth::initAllCerts()
{
    allCerts = QSslCertificate::fromPath(":/resources/all_certs.pem");
}

void Metin2Auth::initPrivateKey()
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

    // this->privateKey = QSslKey(key, QSsl::Rsa);
    this->privateKey = QSslKey(key, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey, "Iroo1pasaife6Zax9caijiebaHuH9aeh");
}

void Metin2Auth::initGfVersion()
{
    QJsonObject jsonResponse;
    QNetworkRequest request(QUrl("http://dl.tnt.gameforge.com/tnt/final-ms3/clientversioninfo.json"));
    QNetworkReply* reply = nullptr;

    reply = networkManager->get(request);
    reply->deleteLater();

    QByteArray response = reply->readAll();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return;

    jsonResponse = QJsonDocument::fromJson(response).object();

    this->chromeVersion = "C" + jsonResponse["version"].toString();
    this->gameforgeVersion = jsonResponse["minimumVersionForDelayedUpdate"].toString();
}

void Metin2Auth::setToken(const QString &newToken)
{
    token = newToken;
}

QString Metin2Auth::getToken() const
{
    return token;
}

QString Metin2Auth::getInstallationId() const
{
    return installationId;
}

SyncNetworAccesskManager *Metin2Auth::getNetworkManager() const
{
    return networkManager;
}

bool Metin2Auth::createGameAccount(const QString &email, const QString& name, const QString &gfLang, QJsonObject &response) const
{
    QJsonObject content;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/user/thin/accounts"));
    QNetworkReply* reply;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("TNT-Installation-Id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());

    identity->update();
    BlackBox blackbox(identity, QJsonValue::Null);

    content["blackbox"] = blackbox.encoded();
    content["displayName"] = name;
    content["email"] = email;
    content["gameEnvironmentId"] = "5401ee5b-1316-41ae-a628-73377b8676ba";
    content["gfLang"] = gfLang;
    content["platformGameId"] = "fab180a3-cd65-4b7e-bd0e-2ef77fd0c258.tr-TR";
    content["region"] = "";

    reply = networkManager->post(request, QJsonDocument(content).toJson());

    response = QJsonDocument::fromJson(reply->readAll()).object();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 201) {
        return false;
    }


    return true;
}

QString Metin2Auth::getProxyPassword() const
{
    return proxyPassword;
}

QString Metin2Auth::getProxyUsername() const
{
    return proxyUsername;
}

QString Metin2Auth::getProxyIp() const
{
    return proxyIp;
}

QString Metin2Auth::getSocksPort() const
{
    return socksPort;
}

bool Metin2Auth::getUseProxy() const
{
    return useProxy;
}
