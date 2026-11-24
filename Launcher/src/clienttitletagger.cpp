#include "clienttitletagger.h"

#include <QDebug>
#include <QTimer>

namespace
{
constexpr int POLL_MS = 2000;
}

ClientTitleTagger::ClientTitleTagger(QObject* parent, DWORD pid, const QString& tag)
    : QObject(parent)
    , pid(pid)
    , tag(tag)
    , process(OpenProcess(SYNCHRONIZE, FALSE, pid))
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ClientTitleTagger::poll);
    timer->start(POLL_MS);
}

ClientTitleTagger::~ClientTitleTagger()
{
    if (process)
        CloseHandle(process);
}

BOOL CALLBACK ClientTitleTagger::enumProc(HWND hwnd, LPARAM lParam)
{
    auto* ctx = reinterpret_cast<EnumContext*>(lParam);

    // NOTE: GetWindowThreadProcessId returns the thread id; the process id
    // comes back through the second parameter.
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid != ctx->pid)
        return TRUE;
    if (!IsWindowVisible(hwnd))
        return TRUE;
    // NOTE: no GW_OWNER filter on purpose. Classic Delphi (which NosTale is)
    // owns the main form through a hidden Application window, so the real
    // game window always has an owner. Tool windows are still skipped below.
    if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return TRUE; // tooltips, dropdowns

    wchar_t title[256];
    if (GetWindowTextW(hwnd, title, 256) == 0)
        return TRUE;

    RECT r;
    if (!GetWindowRect(hwnd, &r))
        return TRUE;

    // the game's main window is the biggest one; the Entwell splash and any
    // helper windows are smaller
    const LONG_PTR area = static_cast<LONG_PTR>(r.right - r.left) * (r.bottom - r.top);
    if (area > ctx->bestArea)
    {
        ctx->best = hwnd;
        ctx->bestArea = area;
    }

    return TRUE;
}

HWND ClientTitleTagger::findGameWindow() const
{
    EnumContext ctx{ pid, nullptr, 0 };
    EnumWindows(enumProc, reinterpret_cast<LPARAM>(&ctx));
    return ctx.best;
}

void ClientTitleTagger::poll()
{
    if (process && WaitForSingleObject(process, 0) == WAIT_OBJECT_0)
    {
        deleteLater(); // game exited
        return;
    }

    const HWND hwnd = findGameWindow();
    if (hwnd == nullptr)
        return; // main window not created yet (or already gone)

    wchar_t buffer[512];
    const int len = GetWindowTextW(hwnd, buffer, 512);
    QString title = QString::fromWCharArray(buffer, len);

    // strip the suffix we applied last time, keep whatever the game set
    if (!appliedSuffix.isEmpty() && title.endsWith(appliedSuffix))
        title.chop(appliedSuffix.size());

    const QString suffix = QStringLiteral(" [") + tag + QLatin1Char(']');
    if (title.endsWith(suffix))
        return; // already tagged

    const QString wanted = title + suffix;
    if (SetWindowTextW(hwnd, reinterpret_cast<const wchar_t*>(wanted.utf16())))
    {
        appliedSuffix = suffix;
        qDebug() << "ClientTitleTagger: tagged window of pid" << pid << "as" << wanted;
    }
}
