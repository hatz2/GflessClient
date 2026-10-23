#include "powcaptchasolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QProcess>
#include <QProgressDialog>
#include <QTimer>

namespace
{
    const QString HELPER_NAME = "PowCaptchaSolver.exe";
    // The widget gives up by itself after 180 s; the helper a little later.
    const int HELPER_TIMEOUT_SECONDS = 200;
    const int DIALOG_TIMEOUT_MS = (HELPER_TIMEOUT_SECONDS + 30) * 1000;
}

PowCaptchaSolver::PowCaptchaSolver(const QString &challengeId, const QString &scriptUrl, QObject *parent)
    : QObject{parent}
    , challengeId(challengeId)
    , scriptUrl(scriptUrl)
    , locale(QLocale().name().replace("_", "-"))
{
}

void PowCaptchaSolver::setProxy(const QString &host, const QString &port, const QString &username, const QString &password)
{
    useProxy = !host.isEmpty() && !port.isEmpty();
    proxyHost = host;
    proxyPort = port;
    proxyUsername = username;
    proxyPassword = password;
}

QString PowCaptchaSolver::helperPath()
{
    QString path = QDir(QCoreApplication::applicationDirPath()).filePath(HELPER_NAME);

    return QFileInfo::exists(path) ? path : QString();
}

PowCaptchaSolver::Result PowCaptchaSolver::run(QWidget *parent)
{
    Result result;
    QString helper = helperPath();

    if (helper.isEmpty()) {
        result.detail = HELPER_NAME + " was not found next to the launcher. Reinstall Gfless Client or build the helper (see PowCaptchaSolver/README.md).";
        return result;
    }

    // The request goes through stdin so the proxy credentials never show up
    // in a command line.
    QJsonObject request;
    request["challengeId"] = challengeId;
    request["script"] = scriptUrl;
    request["locale"] = locale;
    request["timeout"] = HELPER_TIMEOUT_SECONDS;

    if (useProxy) {
        QJsonObject proxy;
        proxy["host"] = proxyHost;
        proxy["port"] = proxyPort.toInt();
        proxy["user"] = proxyUsername;
        proxy["password"] = proxyPassword;
        request["proxy"] = proxy;
    }

    QProcess process;
    process.setProgram(helper);
    process.setWorkingDirectory(QFileInfo(helper).absolutePath());
    process.setProcessChannelMode(QProcess::SeparateChannels);

    QProgressDialog dialog(
        "Gameforge is checking this login in a browser window.\nThis takes a few seconds and needs nothing from you.",
        "Cancel", 0, 0, parent
    );
    dialog.setWindowTitle("Gameforge login check");
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumDuration(0);
    dialog.setAutoClose(false);
    dialog.setAutoReset(false);

    QEventLoop loop;
    bool cancelled = false;

    QObject::connect(&process, &QProcess::finished, &loop, &QEventLoop::quit);
    QObject::connect(&process, &QProcess::errorOccurred, &loop, [&](QProcess::ProcessError) {
        if (process.state() == QProcess::NotRunning)
            loop.quit();
    });
    QObject::connect(&dialog, &QProgressDialog::canceled, &loop, [&]() {
        cancelled = true;
        loop.quit();
    });
    QTimer::singleShot(DIALOG_TIMEOUT_MS, &loop, &QEventLoop::quit);

    process.start();

    if (!process.waitForStarted(10000)) {
        result.detail = "could not start " + HELPER_NAME + ".";
        return result;
    }

    process.write(QJsonDocument(request).toJson(QJsonDocument::Compact));
    process.closeWriteChannel();

    dialog.show();

    if (process.state() != QProcess::NotRunning)
        loop.exec();

    dialog.hide();

    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(5000);
        result.detail = cancelled ? "cancelled." : "the check did not finish in time.";
        return result;
    }

    // The verdict is the last line of stdout: {"status": ..., "detail": ...}.
    QByteArray output = process.readAllStandardOutput().trimmed();
    QByteArray lastLine = output.mid(output.lastIndexOf('\n') + 1);
    QJsonObject verdict = QJsonDocument::fromJson(lastLine).object();

    result.solved = verdict.value("status").toString() == "solved";
    result.detail = verdict.value("detail").toString();

    if (!result.solved && result.detail.isEmpty())
        result.detail = QString("the helper ended with exit code %1.").arg(process.exitCode());

    return result;
}
