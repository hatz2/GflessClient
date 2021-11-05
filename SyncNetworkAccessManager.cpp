#include "syncnetworkaccessmanager.h"

SyncNetworAccesskManager::SyncNetworAccesskManager(QObject *parent) : QNetworkAccessManager(parent)
{

}

QNetworkReply* SyncNetworAccesskManager::post(const QNetworkRequest &request, const QByteArray &data)
{
    QNetworkReply* reply = QNetworkAccessManager::post(request, data);

    connect(reply, &QNetworkReply::errorOccurred, this, [=]
    {
        QString err = reply->errorString();
        qDebug() << "Error: " << err;
    });

    while (!reply->isFinished())
        QApplication::processEvents();

    return reply;
}

QNetworkReply* SyncNetworAccesskManager::get(const QNetworkRequest &request)
{
    QNetworkReply* reply = QNetworkAccessManager::get(request);

    connect(reply, &QNetworkReply::errorOccurred, this, [=]
    {
        QString err = reply->errorString();
        qDebug() << "Error: " << err;
    });

    while (!reply->isFinished())
        QApplication::processEvents();

    return reply;
}
