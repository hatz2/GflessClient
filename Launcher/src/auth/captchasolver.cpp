#include "captchasolver.h"

CaptchaSolver::CaptchaSolver(const QString &challengeId, const QString &lang, SyncNetworAccesskManager* netManager, QObject *parent)
    : QObject{parent}
    , networkManager(netManager)
    , gfChallengeId(challengeId)
    , language(lang)
    , lastUpdated(0)
{

}

bool CaptchaSolver::getChallenge()
{
    QNetworkRequest request(QUrl("https://image-drop-challenge.gameforge.com/challenge/" + gfChallengeId + "/" + language));
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36");
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");

    reply = networkManager->get(request);
    reply->deleteLater();

    QByteArray response = reply->readAll();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    QJsonObject jsonResponse = QJsonDocument::fromJson(response).object();

    lastUpdated = static_cast<quint64>(jsonResponse["lastUpdated"].toDouble());

    return true;
}

bool CaptchaSolver::sendAnswer(int answer)
{
    QJsonObject content, jsonResponse;
    QNetworkRequest request(QUrl("https://image-drop-challenge.gameforge.com/challenge/" + gfChallengeId + "/" + language));
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36");
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");

    content["answer"] = answer;

    reply = networkManager->post(request, QJsonDocument(content).toJson());
    reply->deleteLater();

    QByteArray response = reply->readAll();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return false;

    jsonResponse = QJsonDocument::fromJson(response).object();

    lastUpdated = static_cast<quint64>(jsonResponse["lastUpdated"].toDouble());

    return jsonResponse["status"].toString() == "solved";
}

QImage CaptchaSolver::getTextImage()
{
    QNetworkRequest request(QUrl("https://image-drop-challenge.gameforge.com/challenge/" + gfChallengeId + "/" + language + "/text?" + QString::number(lastUpdated)));
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Accept", "image/webp,image/apng,image/*,*/*;q=0.8");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36");
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");

    reply = networkManager->get(request);
    reply->deleteLater();

    QByteArray response = reply->readAll();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return QImage();

    return QImage::fromData(response);
}

QImage CaptchaSolver::getDragIcons()
{
    QNetworkRequest request(QUrl("https://image-drop-challenge.gameforge.com/challenge/" + gfChallengeId + "/" + language + "/drag-icons?" + QString::number((quint64)lastUpdated)));
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Accept", "image/webp,image/apng,image/*,*/*;q=0.8");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36");
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");

    reply = networkManager->get(request);
    reply->deleteLater();

    QByteArray response = reply->readAll();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return QImage();

    return QImage::fromData(response);
}

QImage CaptchaSolver::getDropTargetImage()
{
    QNetworkRequest request(QUrl("https://image-drop-challenge.gameforge.com/challenge/" + gfChallengeId + "/" + language + "/drop-target?" + QString::number((quint64)lastUpdated)));
    QNetworkReply* reply = nullptr;

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("Accept", "image/webp,image/apng,image/*,*/*;q=0.8");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/72.0.3626.121 Safari/537.36");
    request.setRawHeader("Origin", "spark://www.gameforge.com");
    request.setRawHeader("Connection", "Keep-Alive");

    reply = networkManager->get(request);
    reply->deleteLater();

    QByteArray response = reply->readAll();

    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
        return QImage();

    return QImage::fromData(response);
}
