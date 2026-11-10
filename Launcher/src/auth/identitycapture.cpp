#include "identitycapture.h"
#include "deviceidentity.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProgressDialog>
#include <QStringList>
#include <QTimer>

namespace
{
    const QString HELPER_NAME = "BlackboxExtractor.exe";
    const QString LOG_NAME = "capture-last.log";
    // The helper gives up by itself after this long; the dialog a little later.
    const int HELPER_TIMEOUT_SECONDS = 600;
    const int DIALOG_TIMEOUT_MS = (HELPER_TIMEOUT_SECONDS + 30) * 1000;
    // Only the end of the helper's stderr matters (a traceback ends with the
    // exception), so older text is dropped past this size.
    const qsizetype STDERR_CAP_BYTES = 64 * 1024;
    const int STDERR_LINE_MAX_CHARS = 200;

    const QString WAITING_TEXT =
        "Waiting for the Gameforge client to send its device fingerprint.\n\n"
        "Press Play in the Gameforge client (log in first if it asks you to).\n"
        "This window closes by itself.";

    // What the helper printed and how it ended, written to capture-last.log so
    // a failed capture can be reported.
    struct RunLog
    {
        QString commandLine;
        QString ending;
        QStringList events;
        QByteArray stderrText;
    };

    // The last non-empty line of the helper's stderr, shortened for a message box.
    QString lastStderrLine(const QByteArray& stderrText)
    {
        const QStringList lines = QString::fromUtf8(stderrText).split('\n');

        for (auto it = lines.crbegin(); it != lines.crend(); ++it) {
            QString line = it->trimmed();

            if (line.isEmpty())
                continue;

            if (line.size() > STDERR_LINE_MAX_CHARS)
                line = line.left(STDERR_LINE_MAX_CHARS - 3) + "...";

            return line;
        }

        return QString();
    }

    // Writes capture-last.log (replacing the previous one) and returns the
    // folder that holds it, or an empty string when it could not be written.
    QString writeLog(const RunLog& log, const IdentityCapture::Result& result)
    {
        QString folder = DeviceIdentity::directory();
        QFile file(QDir(folder).filePath(LOG_NAME));

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return QString();

        QString text;
        text += "Time: " + QDateTime::currentDateTime().toString(Qt::ISODate) + "\n";
        text += "Command: " + (log.commandLine.isEmpty() ? QString("(not started)") : log.commandLine) + "\n";
        text += "Ended: " + log.ending + "\n";
        text += "Result: " + (result.captured ? QString("captured") : "not captured: " + result.detail) + "\n";
        text += "\nStandard output events:\n";
        text += log.events.isEmpty() ? QString("(none)\n") : log.events.join("\n") + "\n";
        text += "\nStandard error:\n";
        text += log.stderrText.trimmed().isEmpty() ? QString("(empty)\n") : QString::fromUtf8(log.stderrText) + "\n";

        file.write(text.toUtf8());
        file.close();

        return QDir::toNativeSeparators(folder);
    }
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
    RunLog log;
    bool cancelled = false;

    // Every way out goes through here: the log is always written, and a
    // failure (other than a cancel) tells the user where to find it.
    auto finish = [&]() -> Result {
        QString folder = writeLog(log, result);

        if (!result.captured && !cancelled && !folder.isEmpty()) {
            if (!result.detail.endsWith('.'))
                result.detail += '.';

            result.detail += " See " + LOG_NAME + " in " + folder + ".";
        }

        return result;
    };

    QString helper = helperPath();

    if (helper.isEmpty()) {
        result.detail = HELPER_NAME + " was not found next to the launcher. Reinstall Gfless Client.";
        log.ending = "helper missing";
        return finish();
    }

    // The arguments carry no secrets, so the command line goes to the log as is.
    const QStringList arguments = {"--json", "--once", "--timeout", QString::number(HELPER_TIMEOUT_SECONDS)};
    log.commandLine = "\"" + QDir::toNativeSeparators(helper) + "\" " + arguments.join(' ');

    QProcess process;
    process.setProgram(helper);
    process.setArguments(arguments);
    process.setWorkingDirectory(QFileInfo(helper).absolutePath());
    process.setProcessChannelMode(QProcess::SeparateChannels);

    QProgressDialog dialog(WAITING_TEXT, "Cancel", 0, 0, parent);
    dialog.setWindowTitle("Capture device identity");
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumDuration(0);
    dialog.setAutoClose(false);
    dialog.setAutoReset(false);

    QEventLoop loop;
    QString lastError;
    QByteArray pending;

    // The helper prints one JSON object per line: {"event": "attached" |
    // "spawned" | "waiting" | "reattaching" | "blackbox" | "error", ...}.
    auto handleLine = [&](const QByteArray& rawLine) {
        QByteArray line = rawLine.trimmed();

        if (line.isEmpty() || result.captured)
            return;

        QJsonObject message = QJsonDocument::fromJson(line).object();
        QString event = message.value("event").toString();

        if (event == "blackbox") {
            QString blackbox = message.value("blackbox").toString().trimmed();

            // The fingerprint itself stays out of the log; its length is enough.
            QJsonObject logged = message;
            logged["blackbox"] = QString("<%1 chars>").arg(blackbox.size());
            log.events.append(QString::fromUtf8(QJsonDocument(logged).toJson(QJsonDocument::Compact)));

            if (!blackbox.isEmpty()) {
                result.captured = true;
                result.blackbox = blackbox;
                loop.quit();
            }

            return;
        }

        log.events.append(QString::fromUtf8(line));

        if (event == "error") {
            QString detail = message.value("detail").toString().trimmed();

            if (!detail.isEmpty())
                lastError = detail;
        }
        else if (event == "reattaching") {
            // The Gameforge client updated or re-executed itself; the helper
            // waits for the new one.
            QString detail = message.value("detail").toString().trimmed();
            QString reason = detail.isEmpty() ? QString() : " (" + detail + ")";
            dialog.setLabelText("The Gameforge client restarted" + reason + ". Waiting for it to come back...");
        }
        else if (event == "attached") {
            dialog.setLabelText(WAITING_TEXT);
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

    auto readError = [&]() {
        log.stderrText += process.readAllStandardError();

        if (log.stderrText.size() > STDERR_CAP_BYTES)
            log.stderrText = log.stderrText.right(STDERR_CAP_BYTES);
    };

    QObject::connect(&process, &QProcess::readyReadStandardOutput, &loop, readOutput);
    QObject::connect(&process, &QProcess::readyReadStandardError, &loop, readError);
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
        log.ending = "could not start: " + process.errorString();
        return finish();
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
            log.ending = "killed by the launcher after reporting the blackbox";
        }
        else {
            log.ending = QString("exit code %1").arg(process.exitCode());
        }

        readError();
        return finish();
    }

    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(5000);
        readOutput();
        readError();
        log.ending = cancelled ? "killed by the launcher (cancelled)" : "killed by the launcher (timed out)";
        result.detail = cancelled ? "cancelled." : "the Gameforge client sent no fingerprint in time.";
        return finish();
    }

    // The helper ended by itself: read what is left of its output, including
    // a last line without a trailing newline.
    readOutput();
    handleLine(pending);
    readError();

    if (process.exitStatus() == QProcess::CrashExit)
        log.ending = "crashed (" + process.errorString() + ")";
    else
        log.ending = QString("exit code %1").arg(process.exitCode());

    if (result.captured)
        return finish();

    if (!lastError.isEmpty()) {
        result.detail = lastError;
    }
    else {
        // No reason from the helper (a crash, for example): show the end of
        // its stderr, which usually names the exception.
        QString stderrLine = lastStderrLine(log.stderrText);
        result.detail = QString("the helper ended with exit code %1").arg(process.exitCode());
        result.detail += stderrLine.isEmpty() ? QString(" and gave no reason.") : ": " + stderrLine;
    }

    return finish();
}
