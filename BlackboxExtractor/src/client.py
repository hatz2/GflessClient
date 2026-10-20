import frida
from frida.core import Session
import sys
import os
import argparse
import json
import subprocess
import threading
import time

from blackbox_decode import decode_request

# Exit codes (also documented in README.md).
EXIT_CAPTURED = 0       # at least one blackbox was captured
EXIT_NOTHING = 1        # nothing captured (timeout, Ctrl+C, gfclient.exe closed)
EXIT_FAILED = 2         # could not attach, spawn or load the agent script

# How often the waiting loop wakes up to check the timeout and Ctrl+C.
POLL_SECONDS = 0.25


class Output:
    """Routes human-readable lines and JSON events.

    In --json mode stdout carries only JSON events (one object per line) and
    every human-readable line goes to stderr; otherwise everything is printed
    to stdout as before and no events are written.
    """

    def __init__(self, json_mode: bool) -> None:
        self.json_mode = json_mode
        self._lock = threading.Lock()

    def log(self, text: str = "", error: bool = False) -> None:
        stream = sys.stderr if (self.json_mode or error) else sys.stdout
        with self._lock:
            print(text, file=stream, flush=True)

    def event(self, name: str, **fields) -> None:
        if not self.json_mode:
            return
        line = json.dumps({"event": name, **fields})
        with self._lock:
            sys.stdout.write(line + "\n")
            sys.stdout.flush()

    def error(self, detail: str) -> None:
        self.log(f"[-] {detail}")
        self.event("error", detail=detail)

    def agent_log(self, level: str, text: str) -> None:
        # Mirrors frida's default handler (info -> stdout, the rest -> stderr).
        self.log(text, error=level != "info")


class Capture:
    """State shared between the frida message thread and the waiting loop."""

    def __init__(self, out: Output) -> None:
        self.out = out
        self.count = 0
        self.detach_reason: str | None = None
        # Set whenever something the waiting loop must look at happens.
        self.wake = threading.Event()

    def on_message(self, message: dict, data) -> None:
        out = self.out
        if message["type"] == "error":
            out.error(f"Agent error: {message.get('description')}")
            if message.get("stack"):
                out.log(f"    Stack: {message['stack']}")
            return

        if message["type"] != "send":
            return

        payload = message.get("payload")
        if not isinstance(payload, dict) or not isinstance(payload.get("request"), dict):
            out.error("the agent sent a message without a request")
            return
        path = payload.get("path") or "?"
        request: dict = payload["request"]

        blackbox, error = decode_request(request)
        if error is not None:
            out.error(f"{path}: {error}")
            raw = request.get("blackbox")
            if not out.json_mode and error.startswith("could not decode") and isinstance(raw, str):
                # The encoding changed: show the raw value so the format can be studied.
                out.log("[-] Raw value:")
                out.log(raw)
            return

        self.count += 1
        out.log(f"[+] Blackbox captured from {path}")
        if out.json_mode:
            out.event("blackbox", blackbox=blackbox, path=path)
        else:
            out.log(f"\n[🔑] Blackbox: {blackbox}\n")
            copy_to_clipboard(out, blackbox)
        self.wake.set()

    def on_detached(self, reason, crash=None) -> None:
        self.detach_reason = str(reason)
        self.wake.set()


def copy_to_clipboard(out: Output, text: str) -> None:
    try:
        subprocess.run("clip", input=text, text=True, check=False)
        out.log("[✅] Copied to clipboard\n")
    except Exception as e:
        out.log(f"[-] Error copying to clipboard: {e}")


def find_gfclient(gfclient_path: str | None) -> str | None:
    if gfclient_path and os.path.exists(gfclient_path):
        return gfclient_path

    candidates = [
        "C:/Program Files (x86)/GameforgeClient/gfclient.exe",
        "C:/Program Files/GameforgeClient/gfclient.exe",
        os.path.expandvars("%LOCALAPPDATA%/GameforgeClient/gfclient.exe"),
    ]

    for path in candidates:
        if os.path.exists(path):
            return path

    return gfclient_path


def try_attach(out: Output) -> tuple[Session | None, str | None]:
    """Return (session, None) or (None, reason)."""
    try:
        session = frida.attach("gfclient.exe")
        out.log(f"[+] Attached to existing gfclient.exe process (PID: {session.pid})")
        return session, None
    except frida.ProcessNotFoundError as e:
        return None, f"gfclient.exe is not running ({e})"
    except Exception as e:
        out.log(f"[-] Error attaching: {e}")
        return None, f"could not attach to gfclient.exe ({e})"


def try_spawn(out: Output, gfclient_path: str | None) -> tuple[Session | None, str | None]:
    """Return (session, None) or (None, reason)."""
    if not gfclient_path or not os.path.exists(gfclient_path):
        return None, f"gfclient.exe not found at: {gfclient_path}"

    try:
        out.log(f"[*] Spawning {gfclient_path}...")
        pid = frida.spawn(gfclient_path)
        session = frida.attach(pid)
        frida.resume(pid)
        out.log(f"[+] Spawned and attached to PID: {pid}")
        return session, None
    except Exception as e:
        out.log(f"[-] Error spawning: {e}")
        return None, f"could not spawn gfclient.exe ({e})"


def _find_script() -> str | None:
    meipass = getattr(sys, '_MEIPASS', None)
    if getattr(sys, 'frozen', False) and meipass:
        path = os.path.join(meipass, "build", "agent.js")
        if os.path.exists(path):
            return path

    candidates = [
        "build/agent.js",
        os.path.join(os.path.dirname(sys.argv[0]), "build", "agent.js"),
        os.path.join(os.path.dirname(__file__), "..", "build", "agent.js"),
    ]

    for path in candidates:
        if os.path.exists(path):
            return path

    return None


def _make_streams_safe() -> None:
    """Never crash on a character the stream encoding cannot represent.

    Pipes (a GUI running the helper) get UTF-8; consoles keep their encoding.
    """
    for stream in (sys.stdout, sys.stderr):
        try:
            if stream.isatty():
                stream.reconfigure(errors="replace")
            else:
                stream.reconfigure(encoding="utf-8", errors="replace")
        except (AttributeError, ValueError, OSError):
            pass


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="BlackboxExtractor",
        description="Extracts the blackbox field from the auth requests of gfclient.exe"
    )
    parser.add_argument(
        "gfclient_path",
        nargs="?",
        default=None,
        help="Path to gfclient.exe (optional, auto-detected if not provided)"
    )
    attach_mode = parser.add_mutually_exclusive_group()
    attach_mode.add_argument(
        "--spawn", action="store_true",
        help="Force spawn gfclient.exe instead of attaching"
    )
    attach_mode.add_argument(
        "--no-spawn", action="store_true",
        help="Attach only; exit with code 2 if gfclient.exe is not running"
    )
    parser.add_argument(
        "--json", action="store_true",
        help="Write only JSON events (one per line) to stdout; log lines go to stderr"
    )
    parser.add_argument(
        "--once", action="store_true",
        help="Exit as soon as the first blackbox was captured"
    )
    parser.add_argument(
        "--timeout", type=float, default=None, metavar="SECONDS",
        help="Exit with code 1 when nothing was captured within SECONDS (default: wait forever)"
    )
    args = parser.parse_args()
    if args.timeout is not None and args.timeout < 0:
        parser.error("--timeout must not be negative")
    return args


def main() -> int:
    _make_streams_safe()
    args = parse_args()
    out = Output(args.json)

    script_path = _find_script()
    if not script_path:
        out.error("Script file 'build/agent.js' not found; run 'npm run build' first")
        return EXIT_FAILED

    with open(script_path, "r", encoding="utf-8") as f:
        script_code = f.read()

    session = None

    if not args.spawn:
        session, attach_error = try_attach(out)
        if session:
            out.event("attached", pid=session.pid)
        elif args.no_spawn:
            out.error(attach_error or "could not attach to gfclient.exe")
            return EXIT_FAILED

    if not session:
        session, spawn_error = try_spawn(out, find_gfclient(args.gfclient_path))
        if not session:
            out.error(f"Could not attach to or spawn gfclient.exe: {spawn_error}")
            return EXIT_FAILED
        out.event("spawned", pid=session.pid)

    capture = Capture(out)
    session.on("detached", capture.on_detached)
    script = session.create_script(script_code)
    script.set_log_handler(out.agent_log)
    script.on("message", capture.on_message)

    try:
        script.load()
    except Exception as e:
        out.error(f"Error loading script: {e}")
        try:
            session.detach()
        except Exception:
            pass
        return EXIT_FAILED

    out.log("[+] Script loaded successfully")
    out.log("[*] Waiting for auth requests: log in to the Gameforge client, or press Play if it is already logged in...")
    out.log("[*] Press Ctrl+C to exit\n")
    out.event("waiting")

    # Poll an event instead of blocking on stdin: a GUI running the helper
    # through a pipe may never close stdin, and short waits keep Ctrl+C working.
    deadline = None if args.timeout is None else time.monotonic() + args.timeout
    try:
        while True:
            if args.once and capture.count > 0:
                break
            if capture.detach_reason is not None:
                out.error(f"Detached from gfclient.exe ({capture.detach_reason})")
                break
            wait = POLL_SECONDS
            if deadline is not None:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    if capture.count == 0:
                        out.log(f"[-] Nothing captured within {args.timeout:g} s")
                    break
                wait = min(wait, remaining)
            capture.wake.wait(wait)
            capture.wake.clear()
    except KeyboardInterrupt:
        out.log("\n[*] Detaching...")
    finally:
        if capture.detach_reason is None:
            try:
                session.detach()
            except Exception:
                pass

    return EXIT_CAPTURED if capture.count > 0 else EXIT_NOTHING


if __name__ == "__main__":
    sys.exit(main())
