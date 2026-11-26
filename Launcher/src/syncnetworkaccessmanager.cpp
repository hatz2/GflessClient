#include "syncnetworkaccessmanager.h"
#include <QMessageBox>

SyncNetworAccesskManager::SyncNetworAccesskManager(QObject *parent) : QNetworkAccessManager(parent)
{

}

QNetworkReply* SyncNetworAccesskManager::post(const QNetworkRequest &request, const QByteArray &data)
{
    QNetworkReply* reply = QNetworkAccessManager::post(request, data);

    connect(reply, &QNetworkReply::errorOccurred, this, [=, this]
    {
        handleReplyError(reply);
    });

    while (!reply->isFinished())
        QApplication::processEvents();

    return reply;
}

QNetworkReply* SyncNetworAccesskManager::get(const QNetworkRequest &request)
{
    QNetworkReply* reply = QNetworkAccessManager::get(request);

    connect(reply, &QNetworkReply::errorOccurred, this, [=, this]
    {
        handleReplyError(reply);
    });

    while (!reply->isFinished())
        QApplication::processEvents();

    return reply;
}

QNetworkReply *SyncNetworAccesskManager::sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, QIODevice *data)
{
    QNetworkReply* reply = QNetworkAccessManager::sendCustomRequest(request, verb, data);

    connect(reply, &QNetworkReply::errorOccurred, this, [=, this]
    {
        handleReplyError(reply);
    });

    while (!reply->isFinished())
        QApplication::processEvents();

    return reply;
}

void SyncNetworAccesskManager::handleReplyError(QNetworkReply *reply)
{
    const bool usesSession = reply->request().hasRawHeader("Authorization");
    if (usesSession && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401)
        return;

    qDebug() << "Error code:" << reply->error();
    QString err = reply->errorString();
    QMessageBox::critical(nullptr, "Error", err);
}
