#include "Socks5.h"

#ifdef _WIN32
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#define SOCKET_ERROR (-1)
#endif

#include <cstring>
#include <vector>

namespace {

bool sendAll(Socks5Socket sock, const char* data, size_t size)
{
    size_t sent = 0;
    while (sent < size)
    {
        const int n = send(sock, data + sent, static_cast<int>(size - sent), 0);
        if (n <= 0)
            return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

// recv returns whatever has arrived, which for a handshake reply can be less
// than the whole message, so every read waits for all of it.
bool recvAll(Socks5Socket sock, char* data, size_t size)
{
    size_t read = 0;
    while (read < size)
    {
        const int n = recv(sock, data + read, static_cast<int>(size - read), 0);
        if (n <= 0)
            return false;
        read += static_cast<size_t>(n);
    }
    return true;
}

const char NO_AUTH = 0x00;
const char USER_PASS = 0x02;

}

bool Socks5::openTunnel(Socks5Socket sock, const std::string& username, const std::string& password,
    const std::string& ip, unsigned short port, std::string& error)
{
    // Greeting: both methods are offered, so a proxy without credentials is
    // just as usable as one with them.
    const char greeting[] = { 0x05, 0x02, NO_AUTH, USER_PASS };

    if (!sendAll(sock, greeting, sizeof(greeting)))
    {
        error = "could not send the SOCKS5 greeting";
        return false;
    }

    char chosen[2];

    if (!recvAll(sock, chosen, sizeof(chosen)))
    {
        error = "no answer to the SOCKS5 greeting";
        return false;
    }

    if (chosen[0] != 0x05)
    {
        error = "the proxy did not answer SOCKS5";
        return false;
    }

    if (chosen[1] == USER_PASS)
    {
        if (username.size() > 255 || password.size() > 255)
        {
            error = "the proxy credentials are too long for SOCKS5";
            return false;
        }

        std::vector<char> request;
        request.push_back(0x01); // username/password sub-negotiation version
        request.push_back(static_cast<char>(username.size()));
        request.insert(request.end(), username.begin(), username.end());
        request.push_back(static_cast<char>(password.size()));
        request.insert(request.end(), password.begin(), password.end());

        if (!sendAll(sock, request.data(), request.size()))
        {
            error = "could not send the proxy credentials";
            return false;
        }

        char answer[2];

        if (!recvAll(sock, answer, sizeof(answer)))
        {
            error = "no answer to the proxy credentials";
            return false;
        }

        if (answer[1] != 0x00)
        {
            error = "the proxy rejected the credentials";
            return false;
        }
    }
    else if (chosen[1] != NO_AUTH)
    {
        error = "the proxy asked for an authentication method we do not speak";
        return false;
    }

    unsigned char address[4];

    if (inet_pton(AF_INET, ip.c_str(), address) != 1)
    {
        error = "the destination is not an IPv4 address: " + ip;
        return false;
    }

    const unsigned short networkPort = htons(port);

    char request[10];
    request[0] = 0x05; // SOCKS5
    request[1] = 0x01; // connect, a TCP/IP stream
    request[2] = 0x00; // reserved
    request[3] = 0x01; // IPv4 address
    memcpy(&request[4], address, sizeof(address));
    memcpy(&request[8], &networkPort, sizeof(networkPort));

    if (!sendAll(sock, request, sizeof(request)))
    {
        error = "could not ask the proxy for the tunnel";
        return false;
    }

    // The reply is 4 bytes plus a bound address whose length depends on its
    // type, so the tail is read only once the type is known.
    char reply[4];

    if (!recvAll(sock, reply, sizeof(reply)))
    {
        error = "no answer to the tunnel request";
        return false;
    }

    if (reply[1] != 0x00)
    {
        error = "the proxy refused the tunnel (SOCKS5 reply " + std::to_string(reply[1]) + ")";
        return false;
    }

    size_t tail = 0;

    switch (reply[3])
    {
    case 0x01: tail = 4 + 2; break;   // IPv4 and port
    case 0x04: tail = 16 + 2; break;  // IPv6 and port
    case 0x03:
    {
        char length;
        if (!recvAll(sock, &length, 1))
        {
            error = "truncated answer to the tunnel request";
            return false;
        }
        tail = static_cast<unsigned char>(length) + 2u;
        break;
    }
    default:
        error = "the proxy answered with an unknown address type";
        return false;
    }

    std::vector<char> bound(tail);

    if (!recvAll(sock, bound.data(), tail))
    {
        error = "truncated answer to the tunnel request";
        return false;
    }

    return true;
}
