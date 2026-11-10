#ifndef IDENTITYRANDOMIZER_H
#define IDENTITYRANDOMIZER_H

#include <QJsonObject>

/*
 * Re-rolls every machine-stable fingerprint field of a decoded iovation identity
 * so each generated identity file is a distinct device:
 *   - uuid      the iovation client/device id (the strongest link between identities)
 *   - mem/con   device memory (GB) and CPU thread count
 *   - plugins, fonts, audioC, video, audio, media, permissions, webglFP
 *               browser/hardware profile hashes (random hex, same length/format)
 *   - gpu       a plausible UNMASKED_VENDOR_WEBGL,UNMASKED_RENDERER_WEBGL combo
 *   - width/height a plausible available-screen size (taskbar subtracted)
 *   - audioFP/canvasFP rendering/audio fingerprints (same JSON shape)
 *   - vector/creation/d fresh per-generation ephemeral values (same format the
 *               launcher refreshes on every use anyway)
 * Locale-descriptive fields (tz, lang, osType, osVersion, app, vendor,
 * userAgent, browserEnvMask, v) are preserved so the identity stays coherent
 * with the account's IP/region and looks like the same kind of Windows machine.
 */
class IdentityRandomizer
{
public:
    static QJsonObject randomize(const QJsonObject& identity);

private:
    static QString randomUuid();
    static QString randomVector();
    static QString randomHex(int length, bool upper);
    static bool isHexString(const QString& value);
    static int pickInt(const QList<int>& pool);
};

#endif // IDENTITYRANDOMIZER_H
