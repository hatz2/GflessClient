#pragma once

#include <string>

namespace ProxyMod {
    // Routes the game client's own server connection through a SOCKS5 proxy.
    // Returns false when the hooks could not be installed (the patterns no
    // longer match this client build); the game would then connect from the
    // real IP, so the caller must not let it run.
    bool Initialize(const std::string& ip, unsigned short port, const std::string& username, const std::string& password);
}
