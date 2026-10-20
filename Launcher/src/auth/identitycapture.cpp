#include "identitycapture.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProgressDialog>
#include <QTimer>

namespace
{
    const QString HELPER_NAME = "BlackboxExtractor.exe";
    // The helper gives up by itself after this long; the dialog a little later.
    const int HELPER_TIMEOUT_SECONDS = 600;
    const int DIALOG_TIMEOUT_MS = (HELPER_TIMEOUT_SECONDS + 30) * 1000;
}

IdentityCapture::IdentityCapture(QObject *parent)
    : QObject{parent}
{
}

QString IdentityCapture::helperPath()
{
    QString path = QDir(QCoreApplication::applicationDirPath()).filePath(HELPER_NAME);

    return QFileInfo::exists(path) ? path : QString();
}

IdentityCapture::Result IdentityCapture::run(QWidget *parent)
{
    Result result;
    QString helper = helperPath();

    if (helper.isEmpty()) {
        result.detail = HELPER_NAME + " was not found next to the launcher. Reinstall Gfless Client.";
        return result;
    }

    QProcess process;
    process.setProgram(helper);
    process.setArguments({"--json", "--once", "--timeout", QString::number(HELPER_TIMEOUT_SECONDS)});
    process.setWorkingDirectory(QFileInfo(helper).absolutePath());
    process.setProcessChannelMode(QProcess::SeparateChannels);

    QProgressDialog dialog(
        "Waiting for the Gameforge client to send its device fingerprint.\n\n"
        "Press Play in the Gameforge client (log in first if it asks you to).\n"
        "This window closes by itself.",
        "Cancel", 0, 0, parent
    );
    dialog.setWindowTitle("Capture device identity");
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumDuration(0);
    dialog.setAutoClose(false);
    dialog.setAutoReset(false);

    QEventLoop loop;
    bool cancelled = false;
    QString lastError;
    QByteArray pending;

    // The helper prints one JSON object per line: {"event": "attached" |
    // "spawned" | "waiting" | "blackbox" | "error", ...}.
    auto handleLine = [&](const QByteArray& rawLine) {
        QByteArray line = rawLine.trimmed();

        if (line.isEmpty() || result.captured)
            return;

        QJsonObject message = QJsonDocument::fromJson(line).object();
        QString event = message.value("event").toString();

        if (event == "blackbox") {
            QString blackbox = message.value("blackbox").toString().trimmed();

            if (!blackbox.isEmpty()) {
                result.captured = true;
                result.blackbox = blackbox;
                loop.quit();
            }
        }
        else if (event == "error") {
            QString detail = message.value("detail").toString().trimmed();

            if (!detail.isEmpty())
                lastError = detail;
        }
    };

    auto readOutput = [&]() {
        pending += process.readAllStandardOutput();

        qsizetype newline;

        while ((newline = pending.indexOf('\n')) >= 0) {
            handleLine(pending.left(newline));
            pending.remove(0, newline + 1);
        }
    };

    QObject::connect(&process, &QProcess::readyReadStandardOutput, &loop, readOutput);
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

    dialog.show();

    if (process.state() != QProcess::NotRunning && !result.captured)
        loop.exec();

    dialog.hide();

    if (result.captured) {
        // With --once the helper detaches and exits by itself right after
        // reporting the blackbox.
        if (!process.waitForFinished(5000)) {
            process.kill();
            process.waitForFinished(5000);
        }

        return result;
    }

    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(5000);
        result.detail = cancelled ? "cancelled." : "the Gameforge client sent no fingerprint in time.";
        return result;
    }

    // The helper ended by itself: read what is left of its output, including
    // a last line without a trailing newline.
    readOutput();
    handleLine(pending);

    if (result.captured)
        return result;

    if (!lastError.isEmpty())
        result.detail = lastError;
    else
        result.detail = QString("the Gameforge client sent no fingerprint (exit code %1).").arg(process.exitCode());

    return result;
}
