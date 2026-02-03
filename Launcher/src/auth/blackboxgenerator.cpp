#include "blackboxgenerator.h"
#include "qeventloop.h"
#include "qtimer.h"
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
    page->load(QUrl("qrc:/resources/blackbox.html"));
    page->setWebChannel(channel);
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

QString BlackboxGenerator::generate(const QString &gsid, const QString &installationId)
{
    QEventLoop loop;
    QString result;

    if (BlackboxGenerator::getInstance()->page->isLoading())
        return {};

    connect(getInstance(), &BlackboxGenerator::blackboxCreated, &loop, [&](const QString& blackbox) {
        result = blackbox;
        loop.quit();
    });

    if (gsid.isEmpty() && installationId.isEmpty()) {
        connect(getInstance()->page, &QWebEnginePage::loadFinished, getInstance(), [&](bool ok) {
            if (ok)
                BlackboxGenerator::getInstance()->page->runJavaScript("game1(callbackHandler.callback)");
        });

        BlackboxGenerator::getInstance()->page->runJavaScript("game1(callbackHandler.callback)");
    }
    else {
        QJsonObject request = createRequest(gsid, installationId);
        QString script = QString("game1(callbackHandler.callback, %1)").arg(QJsonDocument(request).toJson());

        connect(getInstance()->page, &QWebEnginePage::loadFinished, getInstance(), [&](bool ok) {
            if (ok)
                BlackboxGenerator::getInstance()->page->runJavaScript(script);
        });

        BlackboxGenerator::getInstance()->page->runJavaScript(script);
    }

    //QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    loop.exec();
    return result;
}

QByteArray BlackboxGenerator::encrypt(const QByteArray &blackbox, const QString &gsid, const QString &accountId)
{
    QByteArray encrypted(blackbox.size(), '\0');
    QByteArray key = gsid.toLocal8Bit() + "-" + accountId.toLocal8Bit();

    key = QCryptographicHash::hash(key, QCryptographicHash::Sha512).toHex();

    for (size_t i = 0; i < blackbox.size(); ++i)
    {
        size_t key_index = i % key.size();
        encrypted[i] = blackbox[i] ^ key[key_index] ^ key[key.size() - key_index - 1];
    }

    return encrypted.toBase64();
}

void BlackboxGenerator::callback(const QString &blackbox)
{
    emit blackboxCreated("tra:" + blackbox);
}
