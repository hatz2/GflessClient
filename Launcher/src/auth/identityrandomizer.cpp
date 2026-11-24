#include "identityrandomizer.h"

#include <QDateTime>
#include <QList>
#include <QPair>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>

#include <cmath>

namespace
{

// v7 blackbox fields (see BlackBox::BLACKBOX_FIELDS) that carry a hex hash of a
// browser/hardware profile. Same length, same case, fresh random content.
const QStringList kHexHashFields = {
    "plugins", "fonts", "audioC", "video", "audio", "media", "permissions", "webglFP"
};

// navigator.deviceMemory is rounded to a power of two and capped at 8.
const QList<int> kDeviceMemoryGb = { 2, 4, 8 };

// hardwareConcurrency values seen on real consumer Windows machines.
const QList<int> kHardwareConcurrency = { 4, 6, 8, 10, 12, 14, 16, 20, 24 };

// screen.availWidth/availHeight: common desktop resolutions minus taskbar.
const QList<QPair<int, int>> kAvailScreens = {
    { 1920, 1040 }, { 1920, 1032 }, { 1920, 1048 }, { 1536, 816 }, { 1536, 802 },
    { 1366, 728 },  { 1600, 860 },  { 1600, 852 },  { 1280, 660 }, { 1280, 652 },
    { 2560, 1320 }, { 2560, 1312 }, { 1680, 1010 }, { 1680, 1002 }, { 1440, 790 },
    { 1440, 782 },  { 3840, 2032 }, { 3840, 2024 }
};

// UNMASKED_VENDOR_WEBGL,UNMASKED_RENDERER_WEBGL as real Chrome/ANGLE reports them.
const QStringList kGpuStrings = {
    "Google Inc. (Intel),ANGLE (Intel, Intel(R) UHD Graphics 620 (0x00003EA5) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (Intel),ANGLE (Intel, Intel(R) UHD Graphics 630 (0x00003E9B) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (Intel),ANGLE (Intel, Intel(R) UHD Graphics 730 (0x00004C8B) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (Intel),ANGLE (Intel, Intel(R) Iris(R) Xe Graphics (0x00009A49) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (NVIDIA),ANGLE (NVIDIA, NVIDIA GeForce GTX 1650 (0x00001F82) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (NVIDIA),ANGLE (NVIDIA, NVIDIA GeForce GTX 1660 SUPER (0x000021C4) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (NVIDIA),ANGLE (NVIDIA, NVIDIA GeForce RTX 2060 (0x00001F08) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (NVIDIA),ANGLE (NVIDIA, NVIDIA GeForce RTX 3060 (0x00002503) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (NVIDIA),ANGLE (NVIDIA, NVIDIA GeForce RTX 3070 (0x00002484) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (AMD),ANGLE (AMD, AMD Radeon(TM) Graphics (0x0000164E) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (AMD),ANGLE (AMD, AMD Radeon RX 580 Series (0x000067DF) Direct3D11 vs_5_0 ps_5_0, D3D11)",
    "Google Inc. (AMD),ANGLE (AMD, AMD Radeon RX 6600 (0x000073FF) Direct3D11 vs_5_0 ps_5_0, D3D11)"
};

// iovation client id: 27 random printable ASCII characters, base64, lower case
// (mirrors Fingerprint::generateUuid()).
// 100 random printable ASCII characters + ' ' + epoch milliseconds, base64
// (mirrors Fingerprint::generateVector()).

} // namespace

QString IdentityRandomizer::randomUuid()
{
    QByteArray chars;
    for (int i = 0; i < 27; ++i)
        chars.push_back(static_cast<char>(QRandomGenerator::global()->bounded(32, 127)));
    return QString::fromLatin1(chars.toBase64().toLower());
}

QString IdentityRandomizer::randomVector()
{
    QByteArray chars;
    for (int i = 0; i < 100; ++i)
        chars.push_back(static_cast<char>(QRandomGenerator::global()->bounded(32, 127)));
    chars.push_back(' ');
    chars += QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    return QString::fromLatin1(chars.toBase64());
}


QJsonObject IdentityRandomizer::randomize(const QJsonObject& identity)
{
    QJsonObject fp = identity;

    if (fp.contains("uuid"))
        fp["uuid"] = randomUuid();

    if (fp.contains("mem"))
        fp["mem"] = pickInt(kDeviceMemoryGb);

    if (fp.contains("con"))
        fp["con"] = pickInt(kHardwareConcurrency);

    for (const QString& field : kHexHashFields)
    {
        const QJsonValue value = fp.value(field);
        if (!value.isString())
            continue;

        const QString current = value.toString();
        if (!isHexString(current))
            continue; // unknown format: keep the captured value

        const bool upper = current == current.toUpper();
        fp[field] = randomHex(current.size(), upper);
    }

    if (fp.value("width").isDouble() && fp.value("height").isDouble())
    {
        const QPair<int, int> screen = kAvailScreens.at(
            QRandomGenerator::global()->bounded(kAvailScreens.size()));
        fp["width"] = screen.first;
        fp["height"] = screen.second;
    }

    if (fp.value("gpu").isString() && fp.value("gpu").toString().contains(QLatin1Char(',')))
        fp["gpu"] = kGpuStrings.at(QRandomGenerator::global()->bounded(kGpuStrings.size()));

    // offline audio fingerprint: a float in the range real machines report
    if (fp.value("audioFP").isDouble())
        fp["audioFP"] = 30.0 + QRandomGenerator64::global()->generateDouble() * 100.0;

    // canvas fingerprint: numeric hash, keep the integer/double shape of the capture
    const QJsonValue canvas = fp.value("canvasFP");
    if (canvas.isDouble())
    {
        const double d = canvas.toDouble();
        if (d == std::floor(d))
            fp["canvasFP"] = static_cast<double>(QRandomGenerator::global()->bounded(0, 2147483647));
        else
            fp["canvasFP"] = QRandomGenerator64::global()->generateDouble() * 2147483647.0;
    }

    // collection timing: same range Identity::updateTimings() refreshes per use
    if (fp.contains("d"))
        fp["d"] = QRandomGenerator64::global()->bounded(150, 301);

    if (fp.contains("vector"))
        fp["vector"] = randomVector();

    if (fp.contains("creation"))
        fp["creation"] = QDateTime::currentDateTime().toUTC().toString(Qt::DateFormat::ISODateWithMs);

    return fp;
}

QString IdentityRandomizer::randomHex(int length, bool upper)
{
    QByteArray bytes;
    bytes.reserve((length + 1) / 2);
    for (int i = 0; i < (length + 1) / 2; ++i)
        bytes.push_back(static_cast<char>(QRandomGenerator::global()->bounded(0, 256)));

    QString hex = QString::fromLatin1(bytes.toHex()).left(length);
    return upper ? hex.toUpper() : hex;
}

bool IdentityRandomizer::isHexString(const QString& value)
{
    if (value.size() < 8)
        return false;

    static const QRegularExpression pattern(QStringLiteral("^[0-9a-fA-F]+$"));
    return pattern.match(value).hasMatch();
}

int IdentityRandomizer::pickInt(const QList<int>& pool)
{
    return pool.at(QRandomGenerator::global()->bounded(pool.size()));
}
