"""Tests Socks5::openTunnel against a real SOCKS5 server.

The handshake is the one part of ProxyMod that does not touch the game, so it
is kept in Socks5.cpp, which compiles on any platform. This test builds it
with the system compiler and runs it against a small proxy implemented here,
so a protocol mistake shows up without a Windows box and the game.

Run it with: python -m unittest discover -s GflessDLL/tests
"""
from __future__ import annotations

import os
import selectors
import shutil
import socket
import subprocess
import tempfile
import threading
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(HERE, os.pardir, "GflessDLL", "Socks5.cpp")
INCLUDE = os.path.join(HERE, os.pardir, "GflessDLL")

# Calls openTunnel on a socket connected to argv[1]:argv[2], asks for a tunnel
# to argv[3]:argv[4] with the credentials in argv[5]/argv[6], then echoes what
# comes back. Prints "ok" or "error: <reason>".
DRIVER = r"""
#include "Socks5.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char** argv)
{
    sockaddr_in proxy{};
    proxy.sin_family = AF_INET;
    proxy.sin_port = htons(static_cast<unsigned short>(atoi(argv[2])));
    inet_pton(AF_INET, argv[1], &proxy.sin_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, reinterpret_cast<sockaddr*>(&proxy), sizeof(proxy)) != 0) {
        printf("error: could not reach the proxy\n");
        return 1;
    }

    std::string error;
    if (!Socks5::openTunnel(sock, argv[5], argv[6], argv[3],
            static_cast<unsigned short>(atoi(argv[4])), error)) {
        printf("error: %s\n", error.c_str());
        return 1;
    }

    // The tunnel is open: anything written now has to reach the destination.
    const char* probe = "ping";
    if (send(sock, probe, strlen(probe), 0) < 0) {
        printf("error: could not write through the tunnel\n");
        return 1;
    }

    char answer[32] = {0};
    ssize_t n = recv(sock, answer, sizeof(answer) - 1, 0);
    if (n <= 0) {
        printf("error: nothing came back through the tunnel\n");
        return 1;
    }

    printf("ok %s\n", answer);
    return 0;
}
"""


def compiler() -> str | None:
    for name in ("c++", "clang++", "g++"):
        found = shutil.which(name)
        if found:
            return found
    return None


class Socks5Server:
    """A SOCKS5 proxy. `credentials` None means it offers no authentication."""

    def __init__(self, credentials: tuple[str, str] | None) -> None:
        self.credentials = credentials
        self.requested: tuple[str, int] | None = None
        self._listener = socket.socket()
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen(8)
        self.port: int = self._listener.getsockname()[1]
        self._closed = threading.Event()
        threading.Thread(target=self._serve, daemon=True).start()

    def close(self) -> None:
        self._closed.set()
        self._listener.close()

    def _serve(self) -> None:
        while not self._closed.is_set():
            try:
                client, _ = self._listener.accept()
            except OSError:
                return
            threading.Thread(target=self._session, args=(client,), daemon=True).start()

    def _read(self, sock: socket.socket, size: int) -> bytes:
        data = b""
        while len(data) < size:
            chunk = sock.recv(size - len(data))
            if not chunk:
                raise ConnectionError("short read")
            data += chunk
        return data

    def _session(self, client: socket.socket) -> None:
        try:
            version, count = self._read(client, 2)
            methods = self._read(client, count)
            if self.credentials is None:
                client.sendall(b"\x05\x00")
            elif 0x02 in methods:
                client.sendall(b"\x05\x02")
                self._read(client, 1)  # sub-negotiation version
                user = self._read(client, self._read(client, 1)[0]).decode()
                password = self._read(client, self._read(client, 1)[0]).decode()
                if (user, password) != self.credentials:
                    client.sendall(b"\x01\x01")
                    return
                client.sendall(b"\x01\x00")
            else:
                client.sendall(b"\x05\xff")
                return

            header = self._read(client, 4)
            address = socket.inet_ntoa(self._read(client, 4))
            port = int.from_bytes(self._read(client, 2), "big")
            self.requested = (address, port)
            assert header[:2] == b"\x05\x01", header

            # Connect where we were asked to and splice the two sockets.
            try:
                upstream = socket.create_connection((address, port), 5)
            except OSError:
                client.sendall(b"\x05\x05\x00\x01" + bytes(6))
                return

            client.sendall(b"\x05\x00\x00\x01" + socket.inet_aton("127.0.0.1")
                           + (0).to_bytes(2, "big"))
            self._splice(client, upstream)
        except (OSError, ConnectionError, AssertionError):
            pass
        finally:
            client.close()

    def _splice(self, a: socket.socket, b: socket.socket) -> None:
        with selectors.DefaultSelector() as selector:
            selector.register(a, selectors.EVENT_READ, b)
            selector.register(b, selectors.EVENT_READ, a)
            while True:
                events = selector.select(timeout=5)
                if not events:
                    return
                for key, _ in events:
                    data = key.fileobj.recv(4096)
                    if not data:
                        return
                    key.data.sendall(data)


class EchoServer:
    """Answers "pong" to anything, so a working tunnel is visible."""

    def __init__(self) -> None:
        self._listener = socket.socket()
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen(8)
        self.port: int = self._listener.getsockname()[1]
        threading.Thread(target=self._serve, daemon=True).start()

    def close(self) -> None:
        self._listener.close()

    def _serve(self) -> None:
        while True:
            try:
                client, _ = self._listener.accept()
            except OSError:
                return
            try:
                client.recv(64)
                client.sendall(b"pong")
            except OSError:
                pass
            finally:
                client.close()


@unittest.skipIf(compiler() is None, "no C++ compiler available")
class Socks5Test(unittest.TestCase):
    driver: str
    _tmp: tempfile.TemporaryDirectory

    @classmethod
    def setUpClass(cls) -> None:
        cls._tmp = tempfile.TemporaryDirectory()
        source = os.path.join(cls._tmp.name, "driver.cpp")
        with open(source, "w") as handle:
            handle.write(DRIVER)
        cls.driver = os.path.join(cls._tmp.name, "driver")
        subprocess.run([compiler(), "-std=c++17", "-I", INCLUDE, "-o", cls.driver,
                        source, SOURCE], check=True)

    @classmethod
    def tearDownClass(cls) -> None:
        cls._tmp.cleanup()

    def run_driver(self, proxy_port: int, host: str, port: int,
                   user: str = "", password: str = "") -> str:
        result = subprocess.run(
            [self.driver, "127.0.0.1", str(proxy_port), host, str(port), user, password],
            capture_output=True, text=True, timeout=30)
        return result.stdout.strip()

    def test_tunnel_through_a_proxy_without_credentials(self):
        echo, proxy = EchoServer(), Socks5Server(None)
        try:
            self.assertEqual(self.run_driver(proxy.port, "127.0.0.1", echo.port), "ok pong")
            self.assertEqual(proxy.requested, ("127.0.0.1", echo.port))
        finally:
            proxy.close()
            echo.close()

    def test_tunnel_through_a_proxy_with_credentials(self):
        echo, proxy = EchoServer(), Socks5Server(("alice", "s3cret"))
        try:
            self.assertEqual(
                self.run_driver(proxy.port, "127.0.0.1", echo.port, "alice", "s3cret"),
                "ok pong")
            self.assertEqual(proxy.requested, ("127.0.0.1", echo.port))
        finally:
            proxy.close()
            echo.close()

    def test_a_wrong_password_is_reported(self):
        echo, proxy = EchoServer(), Socks5Server(("alice", "s3cret"))
        try:
            self.assertIn("rejected the credentials",
                          self.run_driver(proxy.port, "127.0.0.1", echo.port, "alice", "nope"))
        finally:
            proxy.close()
            echo.close()

    def test_a_refused_destination_is_reported(self):
        # A port nothing ever listened on: closing a live server's listener
        # does not necessarily free its accept loop on every platform.
        spare = socket.socket()
        spare.bind(("127.0.0.1", 0))
        dead = spare.getsockname()[1]
        spare.close()
        proxy = Socks5Server(None)
        try:
            self.assertIn("refused the tunnel", self.run_driver(proxy.port, "127.0.0.1", dead))
        finally:
            proxy.close()

    def test_a_proxy_that_is_not_there_is_reported(self):
        spare = socket.socket()
        spare.bind(("127.0.0.1", 0))
        port = spare.getsockname()[1]
        spare.close()
        self.assertIn("could not reach the proxy", self.run_driver(port, "127.0.0.1", 80))


if __name__ == "__main__":
    unittest.main()
