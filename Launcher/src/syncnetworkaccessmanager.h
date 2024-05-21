#ifndef SYNCNETWORKACCESSMANAGER_H
#define SYNCNETWORKACCESSMANAGER_H

#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>

class SyncNetworAccesskManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit SyncNetworAccesskManager(QObject *parent = nullptr);

    QNetworkReply* post(const QNetworkRequest& request, const QByteArray& data);

    QNetworkReply* get(const QNetworkRequest& request);

    QNetworkReply* sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, QIODevice *data = nullptr);
};

#endif // SYNCNETWORKACCESSMANAGER_H
