#ifndef NOSTALEAUTH_H
#define NOSTALEAUTH_H

#include "QSyncNetworkManager.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QMessageBox>
#include <QApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QUuid>
#include <QRandomGenerator>
#include <QSslKey>

class NostaleAuth : public QObject
{
    Q_OBJECT
public:
    explicit NostaleAuth(QObject *parent = nullptr, QString locale = "en_EN");

    QMap<QString /* displayName */, QString /* id */> getAccounts();

    bool authenthicate(const QString& email, const QString& password);

    QString getToken(const QString& accountId);

signals:

private:
    QString locale;
    QString installationId;
    QString chromeVersion;
    QString gameforgeVersion;
    QString cert;
    QSslKey privateKey;
    QList<QSslCertificate> allCerts;
    QString token;
    QString browserUserAgent;


    QChar getFirstNumber(QString uuid);
    QByteArray generateSecondTypeUserAgentMagic();
    QByteArray generateThirdTypeUserAgentMagic(QString accountId);
    bool sendStartTime();
    void initInstallationId();
    void initCert();
    void initAllCerts();
    void initPrivateKey();

};

#endif // NOSTALEAUTH_H
