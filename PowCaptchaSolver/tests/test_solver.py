"""Tests of the helper's pieces that need no browser:
    python -m unittest discover -s tests
"""

import http.client
import io
import json
import os
import socket
import sys
import threading
import time
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

import pow_captcha_solver as solver  # noqa: E402


def _read_exactly(sock, count):
    data = b""
    while len(data) < count:
        chunk = sock.recv(count - len(data))
        if not chunk:
            raise OSError("closed")
        data += chunk
    return data


class EchoServer:
    """Echoes whatever it receives."""

    def __init__(self):
        self.listener = socket.socket()
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen(4)
        self.port = self.listener.getsockname()[1]
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        while True:
            try:
                client, _ = self.listener.accept()
            except OSError:
                return
            threading.Thread(target=self._echo, args=(client,), daemon=True).start()

    @staticmethod
    def _echo(client):
        with client:
            while True:
                data = client.recv(4096)
                if not data:
                    return
                client.sendall(data)

    def close(self):
        self.listener.close()


class FakeSocksProxy:
    """A SOCKS5 proxy that demands a user and password and only lets the
    client reach `target_port` on 127.0.0.1, whatever name it asked for."""

    def __init__(self, user, password, target_port):
        self.user, self.password, self.target_port = user, password, target_port
        self.names = []
        self.listener = socket.socket()
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen(4)
        self.port = self.listener.getsockname()[1]
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        while True:
            try:
                client, _ = self.listener.accept()
            except OSError:
                return
            threading.Thread(target=self._serve, args=(client,), daemon=True).start()

    def _serve(self, client):
        with client:
            version, count = _read_exactly(client, 2)
            methods = _read_exactly(client, count)
            if version != 5 or 2 not in methods:
                client.sendall(b"\x05\xff")
                return
            client.sendall(b"\x05\x02")
            _, user_len = _read_exactly(client, 2)
            user = _read_exactly(client, user_len).decode()
            password = _read_exactly(client, _read_exactly(client, 1)[0]).decode()
            if (user, password) != (self.user, self.password):
                client.sendall(b"\x01\x01")
                return
            client.sendall(b"\x01\x00")
            _, command, _, kind = _read_exactly(client, 4)
            assert command == 1 and kind == 3
            self.names.append(_read_exactly(client, _read_exactly(client, 1)[0]).decode())
            _read_exactly(client, 2)
            upstream = socket.create_connection(("127.0.0.1", self.target_port))
            client.sendall(b"\x05\x00\x00\x01" + bytes(6))
            threading.Thread(target=self._pump, args=(upstream, client), daemon=True).start()
            self._pump(client, upstream)

    @staticmethod
    def _pump(source, target):
        try:
            while True:
                data = source.recv(4096)
                if not data:
                    return
                target.sendall(data)
        except OSError:
            pass
        finally:
            try:
                target.shutdown(socket.SHUT_WR)
            except OSError:
                pass

    def close(self):
        self.listener.close()


def socks5_connect_no_auth(port, host, target_port):
    """What Chromium does against the relay: SOCKS5 CONNECT without credentials."""
    sock = socket.create_connection(("127.0.0.1", port))
    sock.sendall(b"\x05\x01\x00")
    assert _read_exactly(sock, 2) == b"\x05\x00"
    name = host.encode()
    sock.sendall(b"\x05\x01\x00\x03" + bytes([len(name)]) + name + target_port.to_bytes(2, "big"))
    reply = _read_exactly(sock, 10)
    return sock, reply[1]


class RelayTest(unittest.TestCase):
    def test_relay_forwards_through_a_proxy_with_credentials(self):
        echo = EchoServer()
        proxy = FakeSocksProxy("alice", "s3cret", echo.port)
        relay = solver.SocksRelay(solver.Proxy("127.0.0.1", proxy.port, "alice", "s3cret"))
        try:
            sock, reply = socks5_connect_no_auth(relay.port, "pow-captcha.gameforge.com", 443)
            self.assertEqual(reply, 0)
            sock.sendall(b"hello through the relay")
            self.assertEqual(_read_exactly(sock, 23), b"hello through the relay")
            sock.close()
            # The name went to the proxy untouched: nothing was resolved here.
            self.assertEqual(proxy.names, ["pow-captcha.gameforge.com"])
        finally:
            relay.close()
            proxy.close()
            echo.close()

    def test_relay_reports_a_refused_upstream(self):
        echo = EchoServer()
        proxy = FakeSocksProxy("alice", "s3cret", echo.port)
        relay = solver.SocksRelay(solver.Proxy("127.0.0.1", proxy.port, "alice", "wrong"))
        try:
            _, reply = socks5_connect_no_auth(relay.port, "example.com", 443)
            self.assertEqual(reply, 5)       # connection refused
        finally:
            relay.close()
            proxy.close()
            echo.close()

    def test_relay_without_proxy_connects_directly(self):
        echo = EchoServer()
        relay = solver.SocksRelay(None)
        try:
            sock, reply = socks5_connect_no_auth(relay.port, "127.0.0.1", echo.port)
            self.assertEqual(reply, 0)
            sock.sendall(b"direct")
            self.assertEqual(_read_exactly(sock, 6), b"direct")
            sock.close()
        finally:
            relay.close()
            echo.close()


class PageTest(unittest.TestCase):
    def test_page_serves_the_challenge_and_takes_the_verdict(self):
        html = solver.PAGE.format(script=solver.WIDGET_ORIGIN + "/widget.js", origin=solver.WIDGET_ORIGIN,
                                  challenge="abc-123", locale="es-ES")
        page = solver.Page(html)
        try:
            conn = http.client.HTTPConnection("127.0.0.1", page.port, timeout=5)
            conn.request("GET", "/")
            body = conn.getresponse().read().decode()
            self.assertIn('challengeId: "abc-123"', body)
            self.assertIn('locale: "es-ES"', body)
            self.assertIn("Gfless Client", body)
            conn.request("POST", "/done", body=json.dumps({"status": "solved", "detail": "pow-captcha-solved"}),
                         headers={"Content-Type": "application/json"})
            self.assertEqual(conn.getresponse().status, 204)
            self.assertTrue(page.done.wait(2))
            self.assertEqual(page.result, {"status": "solved", "detail": "pow-captcha-solved"})
            self.assertFalse(page.window_gone())        # never seen alive: not gone
            conn.request("POST", "/ping", body="")
            self.assertEqual(conn.getresponse().status, 204)
            self.assertFalse(page.window_gone())
            page.last_ping -= solver.Page.PING_SILENCE + 1
            self.assertTrue(page.window_gone())
        finally:
            page.close()

    def test_page_pings_and_closes_itself_after_reporting(self):
        self.assertIn("window.close()", solver.PAGE)
        self.assertIn('fetch("/ping"', solver.PAGE)


class FakeBrowserTest(unittest.TestCase):
    """run_check with a "browser" that exits at once (python with unknown
    options), as Edge and Chrome do when they relaunch themselves de-elevated."""

    def setUp(self):
        self.pages = []
        original = solver.Page
        pages = self.pages

        class RecordingPage(original):
            def __init__(self, html):
                super().__init__(html)
                pages.append(self)

        self.patches = [patch.object(solver, "Page", RecordingPage),
                        patch.dict(os.environ, {"GFLESS_BROWSER": sys.executable})]
        for p in self.patches:
            p.start()

    def tearDown(self):
        for p in self.patches:
            p.stop()

    def test_an_early_process_exit_is_not_a_closed_window(self):
        with self.assertRaisesRegex(solver.CheckError, "did not finish"):
            solver.run_check("c1", solver.WIDGET_ORIGIN + "/w.js", None, wait_seconds=2)

    def test_a_window_closed_by_hand_is_reported(self):
        def live_then_close():
            # The page pings for a second, then the window goes away.
            time.sleep(0.5)
            conn = http.client.HTTPConnection("127.0.0.1", self.pages[0].port, timeout=5)
            for _ in range(3):
                conn.request("POST", "/ping", body="")
                conn.getresponse().read()
                time.sleep(0.3)

        threading.Thread(target=live_then_close, daemon=True).start()
        started = time.monotonic()
        with self.assertRaisesRegex(solver.CheckError, "closed before"):
            solver.run_check("c1", solver.WIDGET_ORIGIN + "/w.js", None, wait_seconds=30)
        self.assertLess(time.monotonic() - started, 15)


class RequestTest(unittest.TestCase):
    def test_request_fields(self):
        request = solver.read_request(json.dumps({
            "challengeId": "c1", "script": "https://pow-captcha.gameforge.com/w.js", "locale": "de_DE",
            "proxy": {"host": "1.2.3.4", "port": "1080", "user": "alice", "password": "s3cret"}, "timeout": 30}))
        self.assertEqual(request["challenge_id"], "c1")
        self.assertEqual(request["wait_seconds"], 30.0)
        self.assertEqual(request["proxy"], solver.Proxy("1.2.3.4", 1080, "alice", "s3cret"))
        self.assertNotIn("s3cret", repr(request["proxy"]))

    def test_request_without_proxy_and_defaults(self):
        request = solver.read_request('{"challengeId": "c1", "script": "https://pow-captcha.gameforge.com/w.js"}')
        self.assertIsNone(request["proxy"])
        self.assertEqual(request["locale"], "en-GB")
        self.assertEqual(request["wait_seconds"], solver.WAIT_SECONDS)

    def test_bad_requests(self):
        for text in ("not json", "[]", '{"script": "x"}',
                     '{"challengeId": "c", "script": "s", "timeout": "soon"}',
                     '{"challengeId": "c", "script": "s", "proxy": {"host": "h", "port": "x"}}'):
            with self.assertRaises(solver.CheckError):
                solver.read_request(text)

    def test_only_gameforge_scripts_are_loaded(self):
        with self.assertRaises(solver.CheckError):
            solver.run_check("c1", "https://evil.example/widget.js", None)

    def test_widget_locale(self):
        self.assertEqual(solver.widget_locale("es_ES"), "es-ES")
        self.assertEqual(solver.widget_locale("ca-ES"), "en-GB")

    def test_browser_command_routes_through_the_relay(self):
        command = solver.browser_command("browser.exe", "C:/tmp/p", "http://127.0.0.1:1/", 4321)
        self.assertIn("--proxy-server=socks5://127.0.0.1:4321", command)
        self.assertTrue(any(arg.startswith("--host-resolver-rules=") for arg in command))
        self.assertNotIn("--proxy-server=socks5://127.0.0.1:4321",
                         solver.browser_command("browser.exe", "C:/tmp/p", "http://127.0.0.1:1/", None))

    def test_main_reports_a_bad_request_on_stdout(self):
        sys.stdin = io.StringIO("{}")
        out = io.StringIO()
        try:
            with redirect_stdout(out):
                code = solver.main([])
        finally:
            sys.stdin = sys.__stdin__
        self.assertEqual(code, solver.EXIT_ERROR)
        self.assertEqual(json.loads(out.getvalue())["status"], "error")


if __name__ == "__main__":
    unittest.main()
