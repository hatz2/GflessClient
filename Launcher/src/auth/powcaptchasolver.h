#ifndef POWCAPTCHASOLVER_H
#define POWCAPTCHASOLVER_H

#include <QObject>
#include <QString>
#include <QWidget>

// Runs Gameforge's "gf-pow-captcha" challenge. It is not a puzzle for the
// user: Gameforge's own script (the challenge's `script` URL) does a proof of
// work and a set of browser checks by itself, so it needs a real browser.
// The PowCaptchaSolver.exe helper next to the launcher opens a small Edge or
// Chrome window on a local page that embeds that script, routed through the
// account's proxy, and reports the verdict; this class drives the helper and
// shows a progress dialog meanwhile.
class PowCaptchaSolver : public QObject
{
    Q_OBJECT
public:
    struct Result
    {
        bool solved = false;
        QString detail;     // why it did not pass, worded for the user
    };

    explicit PowCaptchaSolver(const QString& challengeId, const QString& scriptUrl, QObject *parent = nullptr);

    void setProxy(const QString& host, const QString& port, const QString& username, const QString& password);

    // The helper's path next to the launcher, or an empty string when it is missing.
    static QString helperPath();

    // Blocks (with a cancellable progress dialog) until the helper reports.
    Result run(QWidget* parent);

private:
    QString challengeId;
    QString scriptUrl;
    QString locale;
    bool useProxy = false;
    QString proxyHost;
    QString proxyPort;
    QString proxyUsername;
    QString proxyPassword;
};

#endif // POWCAPTCHASOLVER_H
