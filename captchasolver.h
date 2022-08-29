#ifndef CAPTCHASOLVER_H
#define CAPTCHASOLVER_H

#include "syncnetworkaccessmanager.h"
#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImage>

class CaptchaSolver : public QObject
{
    Q_OBJECT
public:
    explicit CaptchaSolver(const QString& challengeId, const QString& lang, QObject *parent = nullptr);

    bool getChallenge();

    bool sendAnswer(int answer);

    QImage getTextImage();

    QImage getDragIcons();

    QImage getDropTargetImage();

private:
    QString gfChallengeId;
    QString language;
    quint64 lastUpdated;
};

#endif // CAPTCHASOLVER_H
