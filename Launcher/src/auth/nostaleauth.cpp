#include "nostaleauth.h"
#include "blackbox.h"
#include "blackboxgenerator.h"
#include <QNetworkProxy>
#include <QtZlib/zlib.h>

NostaleAuth::NostaleAuth(const QString &identityPath, const QString& installationID, bool proxy, const QString &proxyHost, const QString &proxyPort, const QString &proxyUser, const QString &proxyPasswd, QObject *parent)
    : QObject(parent)
    , installationId(installationID)
    , proxyIp(proxyHost)
    , socksPort(proxyPort)
    , proxyUsername(proxyUser)
    , proxyPassword(proxyPasswd)
    , useProxy(proxy)
{
    if (identityPath.isEmpty()) {
        identity = nullptr;
    }
    else {
        identity = std::make_shared<Identity>(identityPath, proxyHost, proxyPort, proxyUser, proxyPasswd, proxy);
    }


    this->locale = QLocale().name().replace("_", "-");
    this->browserUserAgent = "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36";
    this->eventsSessionId = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);

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
QByteArray decompressGzip(const QByteArray &data) {
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

QMap<QString, QString> NostaleAuth::getAccounts()
{
    if (!getUserInformation()) {
        return {};
    }

    // if (!sendSystemInformation()) {
    //     return {};
    // }

    // if (!sendStartTime()) {
    //     return {};
    // }

    QJsonObject jsonResponse;
    QMap<QString, QString> accounts;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/user/accounts"));
    QNetworkReply* reply = nullptr;

    if (token.isEmpty())
        return {};

    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("TNT-Installation-Id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");

    reply = networkManager->get(request);

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return {};

    QByteArray replyContent = reply->readAll();
    jsonResponse = QJsonDocument::fromJson(replyContent).object();

    for (const auto& key : jsonResponse.keys())
    {
        auto accountData = jsonResponse[key].toObject();
        auto guls = accountData["guls"].toObject();

        if (guls["game"].toString() != "nostale")
            continue;

        accounts.insert(accountData["id"].toString(), accountData["displayName"].toString());
    }

    reply->deleteLater();

    return accounts;
}

bool NostaleAuth::authenticate(const QString &email, const QString &password, bool& captcha, QString& gfChallengeId, bool &wrongCredentials)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(
        QUrl("https://spark.gameforge.com/api/v2/authProviders/credentials/sessions"));
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.9");
    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("tnt-installation-id", installationId.toUtf8());
    request.setRawHeader("gf-installation-id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept-Encoding", "gzip, deflate, br");

    if (identity == nullptr) {

    }

    content["blackbox"] = createBlackbox();
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

QString NostaleAuth::getToken(const QString &accountId)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/auth/thin/codes"));
    QNetworkReply* reply = nullptr;

    generateGameSessionId();

    if (token.isEmpty()) {
        return {};
    }

    if (!sendIovation(accountId)) {
        return {};
    }

    // if (!sendGameLaunch(accountId)) {
    //     return {};
    // }

    // if (!sendGameStarted(accountId)) {
    //     return {};
    // }

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "Chrome/" + chromeVersion.toLocal8Bit() + " (" + generateThirdTypeUserAgentMagic(accountId) + ")");
    request.setRawHeader("Authorization", "Bearer " + token.toLocal8Bit());
    request.setRawHeader("Connection", "Keep-Alive");
    request.setRawHeader("TNT-installation-id", installationId.toLocal8Bit());
    request.setRawHeader("accept-encoding", "gzip, deflate, br");

    QString gsid = gameSessionId + "-" + QString::number(QRandomGenerator::global()->bounded(1000, 9999));

    content["platformGameAccountId"] = accountId;
    content["gsid"] = gsid;
    content["blackbox"] = QJsonObject();
    //content["blackbox"] = createEncryptedBlackbox(gsid, installationId, accountId);
    content["gameId"] = gameId;

    reply = networkManager->post(request, QJsonDocument(content).toJson());

    QByteArray response = reply->readAll();

    qDebug() << "NostaleAuth::getToken" << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 201)
        return {};

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

bool NostaleAuth::getUserInformation()
{
    QJsonObject jsonResponse;
    QMap<QString, QString> accounts;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v1/user/me"));
    QNetworkReply* reply = nullptr;

    if (token.isEmpty())
        return false;

    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("TNT-Installation-Id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");

    reply = networkManager->get(request);

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    QByteArray replyContent = reply->readAll();
    jsonResponse = QJsonDocument::fromJson(replyContent).object();

    this->gfAccountId = jsonResponse["id"].toString();
    this->locale = jsonResponse["locale"].toString();
    this->email = jsonResponse["email"].toString();

    return true;
}

bool NostaleAuth::sendStartTime()
{
    QJsonObject payload, clientVersionInfo;
    QNetworkRequest request(QUrl("https://events2.gameforge.com"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Host", "events2.gameforge.com");
    request.setRawHeader("User-Agent", "GameforgeClient/" + gameforgeVersion.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");
    request.setSslConfiguration(getCustomSslConfig(sslConfig));

    clientVersionInfo["branch"] = "master";
    clientVersionInfo["commit_id"] = commitId;
    clientVersionInfo["version"] = version;

    payload["client_installation_id"] = installationId;
    payload["client_locale"] = "gbr_eng";
    payload["client_session_id"] = eventsSessionId;
    payload["client_version_info"] = clientVersionInfo;
    payload["id"] = 0;
    payload["localtime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload["start_count"] = 1;
    payload["start_time"] = QRandomGenerator::global()->bounded(1500, 10000);
    payload["type"] = "start_time";

    reply = networkManager->post(request, QJsonDocument(payload).toJson());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    qDebug() << "NostaleAuth::sendStartTime" << reply->readAll();

    reply->deleteLater();

    return true;
}

bool NostaleAuth::sendSystemInformation()
{
    QJsonObject payload, clientVersionInfo;
    QNetworkRequest request(QUrl("https://events.gameforge.com"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Host", "events.gameforge.com");
    request.setRawHeader("User-Agent", "GameforgeClient/" + gameforgeVersion.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");
    request.setSslConfiguration(getCustomSslConfig(sslConfig));

    clientVersionInfo["branch"] = "master";
    clientVersionInfo["commit_id"] = commitId;
    clientVersionInfo["version"] = version;

    payload["client_installation_id"] = installationId;
    payload["client_locale"] = "gbr_eng";
    payload["client_session_id"] = eventsSessionId;
    payload["client_version_info"] = clientVersionInfo;
    payload["frontendVersion"] = frontendVersion;
    payload["id"] = 0;
    payload["localtime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload["os_architecture"] = "x64";
    payload["ram"] = 12341234;
    payload["system_locale"] = "en-en";
    payload["type"] = "system_information";
    payload["version"] = "10.0.26100";


    reply = networkManager->post(request, QJsonDocument(payload).toJson());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    qDebug() << "NostaleAuth::sendSystemInformation" << reply->readAll();

    reply->deleteLater();

    return true;
}

bool NostaleAuth::sendGameLaunch(const QString& accountId)
{
    QJsonObject payload;
    QNetworkRequest request(QUrl("https://events.gameforge.com"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Host", "events.gameforge.com");
    request.setRawHeader("User-Agent", "GameforgeClient/" + gameforgeVersion.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");

    request.setSslConfiguration(getCustomSslConfig(sslConfig));

    // Body
    payload["client_installation_id"] = installationId;
    payload["client_locale"] = "gbr_eng";
    payload["client_session_id"] = eventsSessionId;
    payload["email_hash"] = QString::fromLocal8Bit(QCryptographicHash::hash(email.toLocal8Bit(), QCryptographicHash::Sha256).toHex().toLower());
    payload["game_account_id"] = accountId;
    payload["game_locale"] = "";
    payload["id"] = 0;
    payload["localtime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload["operating_system"] = "windows";
    payload["operating_system_version"] = "10.0.26100";
    payload["product"] = "nostale";
    payload["qwevent"] = "TNT_migration";
    payload["result"] = "success";
    payload["start_option"] = "default_en-GB";
    payload["type"] = "game_launch";
    payload["user_id"] = gfAccountId;

    //qDebug() << "NostaleAuth::sendGameLaunch" << payload;

    reply = networkManager->post(request, QJsonDocument(payload).toJson());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    qDebug() << "NostaleAuth::sendGameLaunch" << reply->readAll();

    reply->deleteLater();

    return true;
}

bool NostaleAuth::sendGameStarted(const QString &accountId)
{
    QJsonObject payload, clientVersionInfo;
    QNetworkRequest request(QUrl("https://events.gameforge.com"));
    QSslConfiguration sslConfig = request.sslConfiguration();
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Host", "events.gameforge.com");
    request.setRawHeader("User-Agent", "GameforgeClient/" + gameforgeVersion.toUtf8());
    request.setRawHeader("Connection", "Keep-Alive");

    request.setSslConfiguration(getCustomSslConfig(sslConfig));

    // Body
    clientVersionInfo["branch"] = "master";
    clientVersionInfo["commit_id"] = commitId;
    clientVersionInfo["version"] = version;

    payload["client_installation_id"] = installationId;
    payload["client_locale"] = "gbr_eng";
    payload["client_session_id"] = eventsSessionId;
    payload["client_version_info"] = clientVersionInfo;
    payload["game_account_id"] = accountId;
    payload["game_id"] = gameId;
    payload["id"] = 0;
    payload["localtime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    payload["session_id"] = gameSessionId;
    payload["start_option"] = "default_en-GB";
    payload["type"] = "game_started";

    //qDebug() << "NostaleAuth::sendGameStarted" << payload;

    reply = networkManager->post(request, QJsonDocument(payload).toJson());

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    qDebug() << "NostaleAuth::sendGameStarted" << reply->readAll();

    reply->deleteLater();

    return true;
}

bool NostaleAuth::sendIovation(const QString& accountId)
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

    content["accountId"] = accountId;
    content["blackbox"] = QJsonObject();
    //content["blackbox"] = createBlackbox();
    content["type"] = "play_now";

    // if (!sendIovationOptions())
    //     return false;

    QByteArray body = QJsonDocument(content).toJson(QJsonDocument::JsonFormat::Compact);
    reply = networkManager->post(request, body);

    reply->deleteLater();

    QByteArray response = reply->readAll();

    qDebug() << " NostaleAuth::sendIovation" << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    jsonResponse = QJsonDocument::fromJson(response).object();

    if (jsonResponse["status"] != "ok")
        return false;

    return true;
}

bool NostaleAuth::sendIovationOptions()
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

    qDebug() << "NostaleAuth::sendIovationOptions" << response;

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 204)
        return false;

    jsonResponse = QJsonDocument::fromJson(response).object();

    return true;
}

void NostaleAuth::initInstallationId()
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
    if (!certFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Error opening cert file " << certFile.errorString();
        return;
    }
    QByteArray certData = certFile.readAll();
    certFile.close();

    this->privateKey = QSslKey(certData, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey, "Iroo1pasaife6Zax9caijiebaHuH9aeh");

    if (this->privateKey.isNull()) {
        qDebug() << "NostaleAuth::initPrivateKey" << "Could not load SSL cert private key";
    }
}

void NostaleAuth::initGfVersion()
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
    this->version = jsonResponse["version"].toString();
    this->gameforgeVersion = jsonResponse["minimumVersionForDelayedUpdate"].toString();
    this->frontendVersion = jsonResponse["frontend"].toObject()["version"].toString();
}

QString NostaleAuth::createBlackbox()
{
    if (identity == nullptr) {
        return BlackboxGenerator::generate();
    }
    else {
        identity->update();
        BlackBox blackbox(identity, QJsonValue::Null);
        return blackbox.encoded();
    }
}

QString NostaleAuth::createEncryptedBlackbox(const QString& gsid, const QString& installationId, const QString& accountId)
{
    if (identity == nullptr) {
        QString blackbox = BlackboxGenerator::generate(gsid, installationId);
        return BlackboxGenerator::encrypt(blackbox.toLocal8Bit(), gsid, accountId);
    }
    else {
        identity->update();
        EncryptedBlackBox blackbox(identity, accountId, gsid, installationId);
        return blackbox.encrypted();
    }
}

QSslConfiguration NostaleAuth::getCustomSslConfig(const QSslConfiguration &config) const
{
    QSslConfiguration sslConfig = config;
    sslConfig.setLocalCertificate(allCerts.first());
    sslConfig.setPrivateKey(privateKey);

    QList<QSslCertificate> caCertificates;
    if (allCerts.size() > 1) {
        caCertificates = allCerts.mid(1);
    }

    // Add system certs to allow fiddler display this requests
    QList<QSslCertificate> trustedCaCerts = QSslConfiguration::systemCaCertificates();
    caCertificates.append(trustedCaCerts);

    sslConfig.setCaCertificates(caCertificates);

    return sslConfig;
}

void NostaleAuth::generateGameSessionId()
{
    this->gameSessionId = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
}

void NostaleAuth::setToken(const QString &newToken)
{
    token = newToken;
}

QString NostaleAuth::getToken() const
{
    return token;
}

QString NostaleAuth::getInstallationId() const
{
    return installationId;
}

SyncNetworAccesskManager *NostaleAuth::getNetworkManager() const
{
    return networkManager;
}

bool NostaleAuth::createGameAccount(const QString &email, const QString& name, const QString &gfLang, QJsonObject &response) const
{
    QJsonObject content;
    QNetworkRequest request(QUrl("https://spark.gameforge.com/api/v2/users/me/accounts"));
    QNetworkReply* reply;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("User-Agent", browserUserAgent.toUtf8());
    request.setRawHeader("tnt-installation-id", installationId.toUtf8());
    request.setRawHeader("gf-installation-id", installationId.toUtf8());
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());

    identity->update();
    BlackBox blackbox(identity, QJsonValue::Null);

    content["blackbox"] = blackbox.encoded();
    content["displayName"] = name;
    //content["email"] = email;
    content["gameEnvironmentId"] = "732876de-012f-4e8d-a501-2e0816cf22f2";
    content["gfLang"] = gfLang;
    content["gameId"] = gameId;

    reply = networkManager->post(request, QJsonDocument(content).toJson());

    response = QJsonDocument::fromJson(reply->readAll()).object();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 201) {
        return false;
    }


    return true;
}

QString NostaleAuth::getProxyPassword() const
{
    return proxyPassword;
}

QString NostaleAuth::getProxyUsername() const
{
    return proxyUsername;
}

QString NostaleAuth::getProxyIp() const
{
    return proxyIp;
}

QString NostaleAuth::getSocksPort() const
{
    return socksPort;
}

bool NostaleAuth::getUseProxy() const
{
    return useProxy;
}
