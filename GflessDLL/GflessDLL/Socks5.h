#pragma once

#include <string>

#ifdef _WIN32
#include <WinSock2.h>
using Socks5Socket = SOCKET;
#else
// Plain sockets, so this file can be compiled and tested off Windows.
using Socks5Socket = int;
#endif

namespace Socks5 {
    // Runs the SOCKS5 client handshake on an already connected socket and asks
    // the proxy to tunnel to ip:port. Offers both "no authentication" and
    // username/password and follows whichever the proxy picks, so proxies
    // without credentials work too.
    //
    // Returns true once the tunnel is open; on failure `error` names the step
    // that failed, and the socket must be treated as unusable.
    bool openTunnel(Socks5Socket sock, const std::string& username, const std::string& password,
        const std::string& ip, unsigned short port, std::string& error);
}
