#include <WinSock2.h>
#include <WS2tcpip.h>
#include "ProxyMod.h"
#include "PatternScanner.h"
#include "Hook.h"
#include "Socks5.h"
#include "GameStructures/NostaleString.h"
#include "GameStructures/TClientCore.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

/*
 * The client resolves and connects to the login/world server itself, so the
 * launcher's own proxy settings never apply to it. Two hooks fix that:
 *
 *  - redirectConnection swaps the ip and port the client is about to connect
 *    to for the proxy's, keeping the original pair aside.
 *  - connectProxy runs right after the socket is connected (to the proxy) and
 *    performs the SOCKS5 handshake for the original destination, so the game
 *    then speaks its own protocol through the tunnel without knowing.
 */

namespace {
    constexpr int HOOK_SIZE = 0x05;
    DWORD connectToServerAddr;
    DWORD connectToServerRetAddr;

    DWORD connectAddr;
    DWORD createConnectionHookAddr;
    DWORD createConnectionRetAddr;

    char* originalIp;
    unsigned int originalPort;

    NostaleStringA proxyIp;
    char* proxyIpAsm;
    unsigned int proxyPort;

    std::string proxyUsername;
    std::string proxyPassword;
}

__declspec(naked) void redirectConnection() {
    __asm {
        // Back up registers in the stack
        pushad;
        pushfd;

        // Back up the original ip and port
        // to send it to the proxy server
        mov originalIp, edx
        mov originalPort, ecx

        // Restore backed up registers
        popfd;
        popad;

        // Change the ip and port for the connection
        mov edx, proxyIpAsm;
        mov ecx, proxyPort;

        // Overwritten original bytes
        push ebx
        push esi
        push edi
        mov edi, ecx

        jmp connectToServerRetAddr;
    }
}

// Kept out of connectProxy: a naked function may hold no object that would
// need unwinding, which rules out std::string (C3068).
static void tunnelConnection()
{
    TClientCore* clientCore = TClientCore::getInstance();
    SOCKET proxySocket = clientCore != nullptr ? clientCore->getSocket() : INVALID_SOCKET;
    std::string error;

    if (proxySocket == INVALID_SOCKET)
        error = "the client's socket could not be read";

    else if (originalIp == nullptr)
        error = "the original destination is unknown";

    else
        Socks5::openTunnel(proxySocket, proxyUsername, proxyPassword,
            originalIp, static_cast<unsigned short>(originalPort), error);

    if (error.empty())
        return;

    // The game is about to speak its own protocol to a tunnel that was never
    // opened. Say so rather than let it fail as a timeout, and close the
    // client: whatever it would reach now, it would reach from the real IP.
    MessageBoxA(NULL, ("The connection could not be tunneled through the proxy: " + error
        + ".\nThe game is being closed.").c_str(), "Gfless Client", MB_ICONERROR);
    ExitProcess(EXIT_FAILURE);
}

__declspec(naked) void connectProxy() {
    __asm {
        // Call the original function first
        call connectAddr

        // Back up the registers in the stack
        pushad;
        pushfd;

        // Prologue for our code
        push ebp
        mov ebp, esp

        call tunnelConnection

        // Epilogue
        mov esp, ebp
        pop ebp

        // Restore backed up registers
        popfd;
        popad;

        jmp createConnectionRetAddr
    }
}

bool ProxyMod::Initialize(const std::string& ip, unsigned short port, const std::string& username, const std::string& password)
{
    proxyIp.set(ip.c_str());
    proxyPort = port;
    proxyIpAsm = proxyIp.get();

    proxyUsername = username;
    proxyPassword = password;

    connectAddr = reinterpret_cast<DWORD>(GetProcAddress(GetModuleHandleA("WS2_32.dll"), "connect"));

    if (connectAddr == 0)
        return false;

    connectToServerAddr = PatternScan(
        "\x53\x56\x57\x8b\xf9\x8b\xf2\x8b\xd8\x8b\xc6\xe8\x00\x00\x00\x00\x50",
        "xxxxxxxxxxxx????x",
        0
    );

    createConnectionHookAddr = PatternScan(
        "\xe8\x00\x00\x00\x00\x40\x74\x00\xc7\x44\x24",
        "x????xx?xxx",
        0
    );

    // A pattern that no longer matches would have us hook address 0. Both
    // hooks are needed: with only the redirect the client would talk its own
    // protocol to the proxy, with only the handshake it would reach the real
    // server directly. Refuse the whole thing instead.
    if (connectToServerAddr == 0 || createConnectionHookAddr == 0)
        return false;

    connectToServerRetAddr = connectToServerAddr + HOOK_SIZE;
    createConnectionRetAddr = createConnectionHookAddr + HOOK_SIZE;

    if (!Hook(reinterpret_cast<void*>(connectToServerAddr), reinterpret_cast<void*>(redirectConnection), HOOK_SIZE))
        return false;

    return Hook(reinterpret_cast<void*>(createConnectionHookAddr), reinterpret_cast<void*>(connectProxy), HOOK_SIZE);
}
