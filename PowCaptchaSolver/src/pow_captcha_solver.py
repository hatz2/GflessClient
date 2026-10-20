"""Runs Gameforge's "gf-pow-captcha" in a real browser for the Gfless launcher.

Since 2026 a login can be answered with a challenge of type `gf-pow-captcha`.
It is not a puzzle for a person: Gameforge's own script (the `script` URL of
the challenge) runs a proof of work and a set of browser checks by itself and
reports the result; in the official client the user only sees "Solving..."
for a few seconds. There is nothing to answer, but it does need a browser.

So this opens a small window of the Edge or Chrome that is installed, with a
throwaway profile, on a local page that embeds that script exactly as the
client does, and waits for it to say it is done. The window's traffic goes
out through the account's proxy like the rest of the login: Chromium cannot
log in to a SOCKS5 proxy with a user and password, so a relay on 127.0.0.1
accepts the browser without credentials and forwards to the real proxy.

The launcher runs this as a helper process:

    PowCaptchaSolver.exe < request.json

`request.json` is one object: {"challengeId": "...", "script": "https://...",
"locale": "en-GB", "proxy": {"host": "...", "port": 1080, "user": "...",
"password": "..."} or null, "timeout": 200}. The verdict is printed as one
JSON line on stdout, {"status": "solved" | "failed" | "error", "detail": ...},
and the exit code is 0 (solved), 1 (the check ran but did not pass) or
2 (it could not run: no browser, bad input, timeout). Progress goes to stderr.
No credentials are ever printed.
"""

from __future__ import annotations

import http.server
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import parse_qs, urlsplit

WIDGET_ORIGIN = "https://pow-captcha.gameforge.com"
# The widget gives up by itself after 180 s.
WAIT_SECONDS = 200.0
LOCALES = ("en-GB", "de-DE", "fr-FR", "es-ES", "pl-PL", "ru-RU", "tr-TR", "pt-BR")

EXIT_SOLVED = 0
EXIT_FAILED = 1
EXIT_ERROR = 2

PAGE = """<!doctype html>
<html><head><meta charset="utf-8"><title>Gfless Client - Gameforge login check</title>
<style>
  body {{ margin: 0; background: #0b111c; color: #b3c0d6;
         font: 14px system-ui, sans-serif; text-align: center; }}
  p {{ margin: 18px 12px 0; }}
</style></head>
<body>
<p id="status">Gameforge is checking this login. This window closes by itself.</p>
<div id="box"></div>
<script src="{script}"></script>
<script>
  let last = "";
  const report = (status, detail) => fetch("/done", {{
      method: "POST", headers: {{"Content-Type": "application/json"}},
      body: JSON.stringify({{status: status, detail: String(detail || "")}})}})
    .catch(() => {{}})
    // This page is the window's only history entry, so it may close itself.
    .finally(() => setTimeout(() => window.close(), 300));
  // A closing browser drops its last requests, so instead of a goodbye the
  // page pings while it lives: the helper knows the window is gone when
  // the pings stop (closed by hand before the verdict, or by itself after).
  const ping = () => fetch("/ping", {{method: "POST"}}).catch(() => {{}});
  ping();
  setInterval(ping, 500);
  // The widget resolves its promise for "solved" and for "failed" alike:
  // the message it posts is what tells them apart.
  window.addEventListener("message", (event) => {{
    if (event.origin === "{origin}" && event.data && event.data.type) last = event.data.type;
  }});
  window.addEventListener("load", () => {{
    const run = window["gf-pow-captcha"];
    if (!run) {{ report("failed", "the widget script did not load"); return; }}
    run(document.getElementById("box"), {{challengeId: "{challenge}", locale: "{locale}"}})
      .then(() => report(last === "pow-captcha-solved" ? "solved" : "failed", last))
      .catch((error) => report("failed", (error && error.message) || error));
  }});
</script>
</body></html>
"""


class CheckError(Exception):
    """The check could not be run (no browser, bad input, timeout)."""


def log(message: str) -> None:
    """Progress lines go to stderr; stdout carries only the verdict."""
    if sys.stderr is not None:
        print(message, file=sys.stderr, flush=True)


# ------------------------------------------------------------------ proxy
@dataclass
class Proxy:
    """A SOCKS5 proxy. The repr never shows the credentials."""

    host: str
    port: int
    user: str = ""
    password: str = ""

    def __repr__(self) -> str:
        return f"Proxy(socks5, {'with' if self.user else 'no'} credentials)"

    @classmethod
    def from_mapping(cls, data: dict | None) -> Proxy | None:
        if not data or not data.get("host") or not data.get("port"):
            return None
        return cls(host=str(data["host"]), port=int(data["port"]),
                   user=str(data.get("user") or ""), password=str(data.get("password") or ""))


def _read_exactly(sock: socket.socket, count: int) -> bytes:
    data = b""
    while len(data) < count:
        chunk = sock.recv(count - len(data))
        if not chunk:
            raise OSError("connection closed")
        data += chunk
    return data


def connect_through_proxy(proxy: Proxy, host: str, port: int, timeout: float) -> socket.socket:
    """A socket connected to host:port through `proxy` (SOCKS5, RFC 1928, with
    the user/password method of RFC 1929 when the proxy has credentials).
    The host name is sent as is, so the proxy resolves it, never this machine."""
    sock = socket.create_connection((proxy.host, proxy.port), timeout)
    try:
        methods = b"\x00\x02" if proxy.user else b"\x00"
        sock.sendall(b"\x05" + bytes([len(methods)]) + methods)
        version, method = _read_exactly(sock, 2)
        if version != 5 or method == 0xFF:
            raise OSError("the proxy accepted none of the authentication methods")
        if method == 2:
            user = proxy.user.encode("utf-8")
            password = proxy.password.encode("utf-8")
            sock.sendall(b"\x01" + bytes([len(user)]) + user + bytes([len(password)]) + password)
            _, status = _read_exactly(sock, 2)
            if status != 0:
                raise OSError("the proxy rejected the credentials")
        elif method != 0:
            raise OSError(f"unexpected authentication method {method}")
        name = host.encode("idna")
        sock.sendall(b"\x05\x01\x00\x03" + bytes([len(name)]) + name + port.to_bytes(2, "big"))
        version, reply, _reserved, kind = _read_exactly(sock, 4)
        if reply != 0:
            raise OSError(f"the proxy could not connect (reply {reply})")
        if kind == 1:
            _read_exactly(sock, 4)
        elif kind == 3:
            _read_exactly(sock, _read_exactly(sock, 1)[0])
        elif kind == 4:
            _read_exactly(sock, 16)
        _read_exactly(sock, 2)
        return sock
    except BaseException:
        sock.close()
        raise


def open_connection(proxy: Proxy | None, host: str, port: int, timeout: float) -> socket.socket:
    if proxy is not None:
        return connect_through_proxy(proxy, host, port, timeout)
    return socket.create_connection((host, port), timeout)


class SocksRelay:
    """SOCKS5 server on 127.0.0.1 without credentials, forwarding every
    connection through `proxy` (or directly when there is none)."""

    def __init__(self, proxy: Proxy | None, timeout: float = 30.0) -> None:
        self._proxy = proxy
        self._timeout = timeout
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen(32)
        self.port: int = self._listener.getsockname()[1]
        self._closed = threading.Event()
        threading.Thread(target=self._accept, daemon=True, name="socks-relay").start()

    def close(self) -> None:
        self._closed.set()
        try:
            self._listener.close()
        except OSError:
            pass

    def _accept(self) -> None:
        while not self._closed.is_set():
            try:
                client, _ = self._listener.accept()
            except OSError:
                return
            threading.Thread(target=self._serve, args=(client,), daemon=True).start()

    def _serve(self, client: socket.socket) -> None:
        upstream: socket.socket | None = None
        try:
            client.settimeout(self._timeout)
            version, methods = _read_exactly(client, 2)
            _read_exactly(client, methods)
            if version != 5:
                return
            client.sendall(b"\x05\x00")                      # no authentication
            version, command, _reserved, kind = _read_exactly(client, 4)
            if kind == 1:
                host = socket.inet_ntoa(_read_exactly(client, 4))
            elif kind == 3:
                host = _read_exactly(client, _read_exactly(client, 1)[0]).decode("idna")
            elif kind == 4:
                host = socket.inet_ntop(socket.AF_INET6, _read_exactly(client, 16))
            else:
                return
            port = int.from_bytes(_read_exactly(client, 2), "big")
            if command != 1:                                  # only CONNECT
                client.sendall(b"\x05\x07\x00\x01" + bytes(6))
                return
            try:
                upstream = open_connection(self._proxy, host, port, self._timeout)
            except OSError:
                client.sendall(b"\x05\x05\x00\x01" + bytes(6))   # connection refused
                return
            client.sendall(b"\x05\x00\x00\x01" + bytes(6))
            client.settimeout(None)
            upstream.settimeout(None)
            pump = threading.Thread(target=self._pump, args=(upstream, client), daemon=True)
            pump.start()
            self._pump(client, upstream)
            pump.join(timeout=5)
        except OSError:
            pass
        finally:
            for sock in (client, upstream):
                if sock is not None:
                    try:
                        sock.close()
                    except OSError:
                        pass

    @staticmethod
    def _pump(source: socket.socket, target: socket.socket) -> None:
        try:
            while True:
                data = source.recv(65536)
                if not data:
                    break
                target.sendall(data)
        except OSError:
            pass
        finally:
            try:
                target.shutdown(socket.SHUT_WR)
            except OSError:
                pass


# ------------------------------------------------------------------- page
class Page:
    """Serves the one page and receives its verdict."""

    # Background windows get their timers slowed down to one tick a second.
    PING_SILENCE = 4.0

    def __init__(self, html: str) -> None:
        self.result: dict | None = None
        self.done = threading.Event()
        self.last_ping: float | None = None
        page = self

        class Handler(http.server.BaseHTTPRequestHandler):
            def log_message(self, *args) -> None:           # keep the console quiet
                pass

            def do_GET(self) -> None:  # noqa: N802 - http.server API
                if urlsplit(self.path).path != "/":
                    self.send_error(404)
                    return
                body = html.encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(body)))
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                self.wfile.write(body)

            def do_POST(self) -> None:  # noqa: N802 - http.server API
                path = urlsplit(self.path).path
                length = min(int(self.headers.get("Content-Length") or 0), 4096)
                raw = self.rfile.read(length)
                if path == "/ping":
                    page.last_ping = time.monotonic()
                    self.send_response(204)
                    self.end_headers()
                    return
                if path != "/done":
                    self.send_error(404)
                    return
                try:
                    data = json.loads(raw.decode("utf-8"))
                except (ValueError, UnicodeDecodeError):
                    data = {k: v[0] for k, v in parse_qs(raw.decode("utf-8", "replace")).items()}
                page.result = {"status": str(data.get("status", "")),
                               "detail": str(data.get("detail", ""))[:200]}
                page.done.set()
                self.send_response(204)
                self.end_headers()

        self._server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.port: int = self._server.server_address[1]
        threading.Thread(target=self._server.serve_forever, daemon=True,
                         name="captcha-page").start()

    def window_gone(self) -> bool:
        """The page was seen alive and has stopped pinging."""
        return self.last_ping is not None and time.monotonic() - self.last_ping > self.PING_SILENCE

    def close(self) -> None:
        self._server.shutdown()
        self._server.server_close()


# ---------------------------------------------------------------- browser
def find_browser() -> str | None:
    """Edge or Chrome (any Chromium works). GFLESS_BROWSER names another one."""
    chosen = os.environ.get("GFLESS_BROWSER")
    if chosen:
        return chosen if Path(chosen).exists() else shutil.which(chosen)
    candidates = []
    for root in (os.environ.get("ProgramFiles(x86)"), os.environ.get("ProgramFiles"),
                 os.environ.get("LocalAppData")):
        if root:
            candidates.append(Path(root) / "Microsoft" / "Edge" / "Application" / "msedge.exe")
            candidates.append(Path(root) / "Google" / "Chrome" / "Application" / "chrome.exe")
    for path in candidates:
        if path.exists():
            return str(path)
    for name in ("msedge", "google-chrome", "chromium", "chromium-browser", "chrome"):
        found = shutil.which(name)
        if found:
            return found
    return None


def browser_command(browser: str, profile: str, url: str, relay_port: int | None) -> list[str]:
    command = [
        browser,
        f"--user-data-dir={profile}",
        "--no-first-run", "--no-default-browser-check", "--disable-sync",
        "--disable-background-networking", "--disable-component-update",
        "--disable-default-apps", "--disable-extensions",
        "--window-size=520,420",
        f"--app={url}",
    ]
    if relay_port is not None:
        command += [
            f"--proxy-server=socks5://127.0.0.1:{relay_port}",
            # Names are resolved by the proxy, never by this machine.
            "--host-resolver-rules=MAP * ~NOTFOUND , EXCLUDE 127.0.0.1",
        ]
    return command


def _no_window() -> dict:
    # This helper may run without a console: child commands must not open one.
    return {"creationflags": subprocess.CREATE_NO_WINDOW} if os.name == "nt" else {}


def _close_browser(process: subprocess.Popen, profile: str) -> None:
    """Takes the browser down, whichever process ended up showing the window."""
    if process.poll() is None:
        try:
            if os.name == "nt":
                # The browser is a tree of processes: take all of it down.
                subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                               capture_output=True, timeout=15, check=False, **_no_window())
            else:
                process.terminate()
            process.wait(timeout=10)
        except (OSError, subprocess.SubprocessError):
            pass
    if os.name == "nt":
        # Started by an elevated program (the launcher runs as administrator),
        # Edge and Chrome relaunch themselves without those rights and the
        # process started here exits at once: the window then belongs to a
        # process that is not in the tree above. It is found by the profile
        # folder, which only this check uses.
        # (The query's own command line names the folder too: it skips itself.)
        script = ("Get-CimInstance Win32_Process | Where-Object { $_.ProcessId -ne $PID -and $_.CommandLine"
                  " -and $_.CommandLine.Contains('" + profile.replace("'", "''") + "') }"
                  " | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }")
        try:
            subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", script],
                           capture_output=True, timeout=30, check=False, **_no_window())
        except (OSError, subprocess.SubprocessError):
            pass


def widget_locale(locale: str) -> str:
    wanted = locale.replace("_", "-")
    return wanted if wanted in LOCALES else "en-GB"


def run_check(challenge_id: str, script_url: str, proxy: Proxy | None,
              locale: str = "en-GB", wait_seconds: float = WAIT_SECONDS) -> dict:
    """Let a browser run the challenge. Returns {"status": "solved" | "failed",
    "detail": ...}; raises CheckError when it could not be run at all."""
    if not script_url.startswith(WIDGET_ORIGIN + "/"):
        # Only Gameforge's own script is ever loaded into the page.
        raise CheckError("unexpected script address for the challenge")
    browser = find_browser()
    if browser is None:
        raise CheckError("no Edge or Chrome found (set GFLESS_BROWSER to a Chromium browser)")

    html = PAGE.format(script=script_url, origin=WIDGET_ORIGIN, challenge=challenge_id,
                       locale=widget_locale(locale))
    page = Page(html)
    relay = SocksRelay(proxy) if proxy is not None else None
    profile = tempfile.mkdtemp(prefix="gfless-check-")
    process = None
    try:
        command = browser_command(browser, profile, f"http://127.0.0.1:{page.port}/",
                                  relay.port if relay else None)
        log("[*] opening a browser window for Gameforge's login check"
            + (" through the proxy" if relay else ""))
        process = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        # The process started here may exit at once while another one shows
        # the window (see _close_browser), so its exit means nothing: the page
        # itself says when its window is closed.
        deadline = time.monotonic() + wait_seconds
        while time.monotonic() < deadline:
            if page.done.wait(0.5):
                break
            if page.window_gone() and not page.done.wait(1.0):
                raise CheckError("the browser window was closed before the check ended")
        if not page.done.is_set():
            raise CheckError(f"the check did not finish in {wait_seconds:.0f} s")
        # Let the window close itself before anything is killed.
        for _ in range(int(Page.PING_SILENCE * 2) + 4):
            if page.window_gone():
                log("[*] the browser window closed by itself")
                break
            time.sleep(0.5)
        return dict(page.result or {"status": "failed", "detail": "no result"})
    finally:
        if process is not None:
            _close_browser(process, profile)
        if relay is not None:
            relay.close()
        page.close()
        for _ in range(10):
            shutil.rmtree(profile, ignore_errors=True)
            if not os.path.exists(profile):
                break
            time.sleep(0.5)


# ------------------------------------------------------------------- main
def read_request(text: str) -> dict:
    """The launcher's request: challengeId and script are required."""
    try:
        data = json.loads(text)
    except ValueError as error:
        raise CheckError(f"the request is not JSON: {error}") from None
    if not isinstance(data, dict):
        raise CheckError("the request must be a JSON object")
    challenge_id = str(data.get("challengeId") or "")
    script = str(data.get("script") or "")
    if not challenge_id or not script:
        raise CheckError("the request needs 'challengeId' and 'script'")
    try:
        timeout = float(data.get("timeout") or WAIT_SECONDS)
    except (TypeError, ValueError):
        raise CheckError("'timeout' must be a number of seconds") from None
    try:
        proxy = Proxy.from_mapping(data.get("proxy"))
    except (TypeError, ValueError, AttributeError):
        raise CheckError("'proxy' needs a host and a numeric port") from None
    return {"challenge_id": challenge_id, "script_url": script, "proxy": proxy,
            "locale": str(data.get("locale") or "en-GB"), "wait_seconds": timeout}


def emit(status: str, detail: str) -> None:
    if sys.stdout is not None:
        print(json.dumps({"status": status, "detail": detail}), flush=True)


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if argv and argv[0] in ("-h", "--help"):
        print(__doc__)
        return EXIT_ERROR
    if sys.stdin is None:
        emit("error", "no request on stdin")
        return EXIT_ERROR
    try:
        request = read_request(sys.stdin.read())
        result = run_check(**request)
    except CheckError as error:
        log(f"[-] {error}")
        emit("error", str(error))
        return EXIT_ERROR
    except KeyboardInterrupt:
        emit("error", "interrupted")
        return EXIT_ERROR
    status = result.get("status", "failed")
    detail = result.get("detail", "")
    log(f"[{'+' if status == 'solved' else '-'}] {status}: {detail}")
    emit(status, detail)
    return EXIT_SOLVED if status == "solved" else EXIT_FAILED


if __name__ == "__main__":
    sys.exit(main())
