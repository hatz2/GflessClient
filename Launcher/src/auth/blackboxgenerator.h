#ifndef BLACKBOXGENERATOR_H
#define BLACKBOXGENERATOR_H

#include <QObject>
#include <QWebEnginePage>
#include <QWebChannel>
#include <QJsonObject>


class BlackboxGenerator : public QObject
{
    Q_OBJECT
public:
    BlackboxGenerator(BlackboxGenerator& other) = delete;
    void operator=(const BlackboxGenerator& other) = delete;

    static BlackboxGenerator* getInstance();

    static void generate(const QString& gsid = "", const QString& installationId = "");

    static QByteArray encrypt(const QByteArray &blackbox, const QString& gsid, const QString& accountId);

    Q_INVOKABLE void callback(const QString& blackbox);

signals:
    void blackboxCreated(QString);

private:
    explicit BlackboxGenerator(QObject *parent = nullptr);

    static QJsonObject createRequest(const QString& gsid, const QString& installationId);


    static BlackboxGenerator* instance;
    QWebEnginePage* page;
    QWebChannel* channel;

};

#endif // BLACKBOXGENERATOR_H
