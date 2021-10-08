#ifndef QSYNCNETWORKMANAGER_H
#define QSYNCNETWORKMANAGER_H

#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>

class QSyncNetworkManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit QSyncNetworkManager(QObject *parent = nullptr);

    QNetworkReply* post(const QNetworkRequest& request, const QByteArray& data);
    QNetworkReply* get(const QNetworkRequest& request);
};

#endif // QSYNCNETWORKMANAGER_H
