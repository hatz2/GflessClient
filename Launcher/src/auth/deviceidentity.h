#ifndef DEVICEIDENTITY_H
#define DEVICEIDENTITY_H

#include <QDateTime>
#include <QString>

// The identity of this PC, captured once from the Gameforge client.
// Two identities captured on the same PC only differ in the fields Gfless
// regenerates at every login, so one capture is enough for every account:
// each account gets its own copy of it (see createForAccount).
class DeviceIdentity
{
public:
    // Folder that holds the device identity and the per-account copies
    // (%APPDATA%\Hatz Nostale\Gfless Client\identities). Created on demand.
    static QString directory();

    static QString devicePath();
    static bool exists();

    // When the device identity was captured; invalid when there is none.
    static QDateTime capturedAt();

    // Decodes a "tra:..." blackbox into the identity JSON and stores it as
    // the device identity, replacing the previous one.
    static bool saveFromBlackbox(const QString& blackbox, QString& error);

    // Returns the path of the account's own identity file, copying the device
    // identity when the account has none yet. An existing file is kept, so an
    // account that is removed and added again keeps its identity. Returns an
    // empty string and sets `error` on failure.
    static QString createForAccount(const QString& email, QString& error);
};

#endif // DEVICEIDENTITY_H
