#include <WinSock2.h>
#include <WS2tcpip.h>
#include "ProxyMod.h"
#include "PatternScanner.h"
#include "Hook.h"
#include "GameStructures/NostaleString.h"
#include "GameStructures/TClientCore.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

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

__declspec(naked) void connectProxy() {
    __asm {
        // Call the original function first
        call connectAddr

        // Back up the registers in the stack
        pushad;
        pushfd;

        // Epilogue for our code
        push ebp
        mov ebp, esp
        sub esp, __LOCAL_SIZE
    }

    {
        SOCKET proxySocket = TClientCore::getInstance()->getSocket();
        int authDetailsSize = 3 + proxyUsername.length() + proxyPassword.length();
        char * authDetails = new char[authDetailsSize];

        // Send the SOCKS5 handshake
        char socks5handshake[] = { 0x05, 0x01, 0x02 };

        if (send(proxySocket, socks5handshake, 3, 0) == SOCKET_ERROR) {
            MessageBoxA(NULL, "Failed to send SOCKS5 handhsake to the proxy server.", "Error", MB_ICONERROR);
            delete[] authDetails;
            goto END;
        }

        // Receive the SOCKS5 handhsake response
        char responseBuffer[2];
        int bytesRead = recv(proxySocket, responseBuffer, 2, 0);

        if (bytesRead <= 0) {
            MessageBoxA(NULL, "Failed to receive SOCKS5 handshake response from the proxy server.", "Error", MB_ICONERROR);
            delete[] authDetails;
            goto END;
        }

        // Check that the response is correct
        if (responseBuffer[0] != 0x05 || responseBuffer[1] != 0x02) {
            MessageBoxA(NULL, "Proxy server does not support SOCKS5 authentication.", "Error", MB_ICONERROR);
            delete[] authDetails;
            goto END;
        }

        if (!proxyUsername.empty()) {
            // Send the SOCKS5 authentication details
            authDetails[0] = 0x01; // Authentication version
            authDetails[1] = static_cast<char>(proxyUsername.length()); // Username length
            memcpy(&authDetails[2], proxyUsername.c_str(), proxyUsername.length()); // Username
            authDetails[2 + authDetails[1]] = static_cast<char>(proxyPassword.length()); // Password length
            memcpy(&authDetails[3 + authDetails[1]], proxyPassword.c_str(), proxyPassword.length()); // Password

            if (send(proxySocket, authDetails, authDetailsSize, 0) == SOCKET_ERROR) {
                MessageBoxA(NULL, "Failed to send SOCKS5 authentication details to the proxy server.", "Error", MB_ICONERROR);
                delete[] authDetails;
                goto END;
            }

            // Receive the SOCKS5 authentication response
            char authResponse[2];
            bytesRead = recv(proxySocket, authResponse, 2, 0);

            if (bytesRead <= 0) {
                MessageBoxA(NULL, "Failed to receive SOCKS5 authentication response.", "Error", MB_ICONERROR);
                delete[] authDetails;
                goto END;
            }

            // Check that the response is correct
            if (authResponse[0] != 0x01 || authResponse[1] != 0x00) {
                MessageBoxA(NULL, "Proxy server rejected SOCKS5 authentication.", "Error", MB_ICONERROR);
                delete[] authDetails;
                goto END;
            }
        }

        // Convert the ip and port to network byte order
        char ip[4];
        if (inet_pton(AF_INET, originalIp, ip) == 0) {
            MessageBoxA(NULL, "Could not convert original IP to network byte order.", "Error", MB_ICONERROR);
            delete[] authDetails;
            goto END;
        }
        u_short port = htons(static_cast<u_short>(originalPort));
        

        // Create the connect request to let the proxy server know the target server
        char socks5ConnectRequest[10];

        socks5ConnectRequest[0] = 0x05; // SOCKS5 version
        socks5ConnectRequest[1] = 0x01; // CMD 0x01 to estabilish a TCP/IP stream connection
        socks5ConnectRequest[2] = 0x00; // Reserved, must be 0
        socks5ConnectRequest[3] = 0x01; // Type of address, IPv4
        memcpy(&socks5ConnectRequest[4], &ip, 4); // IP address in network byte order
        memcpy(&socks5ConnectRequest[8], &port, 2); // Port in network byte order

        // Send the SOCKS5 connection request
        if (send(proxySocket, socks5ConnectRequest, 10, 0) == SOCKET_ERROR) {
            MessageBoxA(NULL, "Failed to send connect request to the proxy server.", "Error", MB_ICONERROR);
            delete[] authDetails;
            goto END;
        }

        // Receive the response
        char connectResponse[10];
        bytesRead = recv(proxySocket, connectResponse, 10, 0);

        if (bytesRead <= 0) {
            MessageBoxA(NULL, "Failed to receive connect response from the proxy server.", "Error", MB_ICONERROR);
            delete[] authDetails;
            goto END;
        }

        // Check if the response is correct
        if (connectResponse[1] != 0x00) {
            MessageBoxA(NULL, "Proxy server rejected SOCKS5 connect request.", "Error", MB_ICONERROR);
        }

        delete[] authDetails;
    }

END:
    __asm {
        // Epilogue
        mov esp, ebp
        pop ebp

        // Restore backed up registers
        popfd;
        popad;

        jmp createConnectionRetAddr
    }
}

void ProxyMod::Initialize(const std::string& ip, unsigned short port, const std::string& username, const std::string& password)
{
    proxyIp.set(ip.c_str());
    proxyPort = port;
    proxyIpAsm = proxyIp.get();

    proxyUsername = username;
    proxyPassword = password;

    connectAddr = reinterpret_cast<DWORD>(GetProcAddress(GetModuleHandleA("WS2_32.dll"), "connect"));

    connectToServerAddr = PatternScan(
        "\x53\x56\x57\x8b\xf9\x8b\xf2\x8b\xd8\x8b\xc6\xe8\x00\x00\x00\x00\x50", 
        "xxxxxxxxxxxx????x",
        0
    );

    connectToServerRetAddr = connectToServerAddr + HOOK_SIZE;
    Hook(reinterpret_cast<void*>(connectToServerAddr), reinterpret_cast<void*>(redirectConnection), HOOK_SIZE);

    createConnectionHookAddr = PatternScan(
        "\xe8\x00\x00\x00\x00\x40\x74\x00\xc7\x44\x24",
        "x????xx?xxx",
        0
    );

    createConnectionRetAddr = createConnectionHookAddr + HOOK_SIZE;

    Hook(reinterpret_cast<void*>(createConnectionHookAddr), reinterpret_cast<void*>(connectProxy), HOOK_SIZE);
}
