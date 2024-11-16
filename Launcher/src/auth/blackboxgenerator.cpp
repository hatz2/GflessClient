#include "blackboxgenerator.h"
#include <QUrl>
#include <QJsonArray>
#include <QRandomGenerator64>
#include <QJsonDocument>

BlackboxGenerator* BlackboxGenerator::instance = nullptr;

BlackboxGenerator::BlackboxGenerator(QObject *parent)
    : QObject{parent}
{
    this->page = new QWebEnginePage(this);
    this->channel = new QWebChannel(page);
    channel->registerObject("callbackHandler", this);
    page->setWebChannel(channel);
    page->load(QUrl("qrc:/resources/blackbox.html"));
}

QJsonObject BlackboxGenerator::createRequest(const QString &gsid, const QString &installationId)
{
    QJsonObject request;
    QJsonArray featuresArray;

    featuresArray.push_back(QRandomGenerator64::global()->bounded(1, INT32_MAX));
    request["features"] = featuresArray;
    request["installation"] = installationId;
    request["session"] = gsid.left(gsid.lastIndexOf("-"));

    return request;
}

BlackboxGenerator *BlackboxGenerator::getInstance()
{
    if (instance == nullptr) {
        instance = new BlackboxGenerator();
    }

    return instance;
}

void BlackboxGenerator::generate(const QString &gsid, const QString &installationId)
{
    if (BlackboxGenerator::getInstance()->page->isLoading())
        return;

    if (gsid.isEmpty() && installationId.isEmpty()) {
        BlackboxGenerator::getInstance()->page->runJavaScript("game1(callbackHandler.callback)");
    }
    else {
        QJsonObject request = createRequest(gsid, installationId);
        QString script = QString("game1(callbackHandler.callback, %1)").arg(QJsonDocument(request).toJson());
        BlackboxGenerator::getInstance()->page->runJavaScript(script);
    }
}

QByteArray BlackboxGenerator::encrypt(const QByteArray &blackbox, const QString &gsid, const QString &accountId)
{
    QByteArray encrypted;
    QByteArray key = gsid.toLatin1() + "-" + accountId.toLatin1();

    key = QCryptographicHash::hash(key, QCryptographicHash::Sha512).toHex();

    for (int i = 0; i < blackbox.size(); ++i)
    {
        int key_index = i % key.size();
        encrypted.push_back(blackbox[i] ^ key[key_index] ^ key[key.size() - key_index - 1]);
    }

    return encrypted.toBase64();
}

void BlackboxGenerator::callback(const QString &blackbox)
{
    emit blackboxCreated("tra:" + blackbox);
}
