#include "QSyncNetworkManager.h"

QSyncNetworkManager::QSyncNetworkManager(QObject *parent) : QNetworkAccessManager(parent)
{

}

QNetworkReply *QSyncNetworkManager::post(const QNetworkRequest &request, const QByteArray &data)
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

QNetworkReply *QSyncNetworkManager::get(const QNetworkRequest &request)
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
