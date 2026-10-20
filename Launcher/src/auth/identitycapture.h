#ifndef IDENTITYCAPTURE_H
#define IDENTITYCAPTURE_H

#include <QObject>
#include <QString>
#include <QWidget>

// Captures this PC's device fingerprint from the Gameforge client. The
// BlackboxExtractor.exe helper next to the launcher attaches to gfclient.exe
// (starting it when it is not running) and reports the first blackbox the
// client sends when the user presses Play. This class drives the helper and
// shows a progress dialog meanwhile.
class IdentityCapture : public QObject
{
    Q_OBJECT
public:
    struct Result
    {
        bool captured = false;
        QString blackbox;   // the "tra:..." string, when captured
        QString detail;     // why nothing was captured, worded for the user
    };

    explicit IdentityCapture(QObject *parent = nullptr);

    // The helper's path next to the launcher, or an empty string when it is missing.
    static QString helperPath();

    // Blocks (with a cancellable progress dialog) until the helper reports.
    Result run(QWidget* parent);
};

#endif // IDENTITYCAPTURE_H
