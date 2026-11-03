import frida
from frida.core import Session
import sys
import os
import argparse
import json
import subprocess
import threading
import time
import traceback

from blackbox_decode import decode_request

# Exit codes (also documented in README.md).
EXIT_CAPTURED = 0       # at least one blackbox was captured
EXIT_NOTHING = 1        # nothing captured (timeout, Ctrl+C, gfclient.exe closed for good)
EXIT_FAILED = 2         # could not attach, spawn or load the agent script; unexpected error

# How often the waiting loop wakes up to check the timeout and Ctrl+C.
POLL_SECONDS = 0.25

# After gfclient.exe went away (it closed, updated or re-executed itself), how
# long to wait for a new one and how often to look for it.
REATTACH_SECONDS = 90
REATTACH_POLL_SECONDS = 2


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


def _is_detached(session: Session) -> bool:
    try:
        return bool(session.is_detached)
    except Exception:
        return False


def _release(session: Session, capture: Capture) -> None:
    """Detach on purpose, without reporting it as the client going away."""
    try:
        session.off("detached", capture.on_detached)
    except Exception:
        pass
    try:
        session.detach()
    except Exception:
        pass


def _attach_script(session: Session, script_code: str, capture: Capture, out: Output) -> bool:
    """Load the agent into the attached gfclient.exe; return False on failure.

    When the client went away while the agent was loading (it updates or
    re-executes itself right after starting), capture.detach_reason is set so
    the caller can wait for the new client instead of giving up.
    """
    try:
        session.on("detached", capture.on_detached)
        script = session.create_script(script_code)
        script.set_log_handler(out.agent_log)
        script.on("message", capture.on_message)
        script.load()
        return True
    except Exception as e:
        failure = f"{type(e).__name__}: {e}"
        if capture.detach_reason is None and _is_detached(session):
            capture.detach_reason = "detached while the agent was loading"
        if capture.detach_reason is not None:
            # Not final: the waiting loop looks for the new client.
            out.log(f"[-] gfclient.exe went away while the agent was loading ({failure})")
        else:
            out.error(f"could not load the agent into gfclient.exe ({failure})")
        _release(session, capture)
        return False


def _announce_waiting(out: Output) -> None:
    out.log("[+] Script loaded successfully")
    out.log("[*] Waiting for auth requests: press Play in the Gameforge client...")
    out.log("[*] Press Ctrl+C to exit\n")
    out.event("waiting")


def _reattach(capture: Capture, script_code: str, out: Output, deadline: float | None) -> Session | None:
    """Wait for a new gfclient.exe after the previous one went away.

    The Gameforge client may update or restart itself when it starts. Polls for
    a new process for up to REATTACH_SECONDS (never past the overall deadline)
    and loads the agent into it. Attach only: a second client is never spawned.
    Returns the new session, or None after an error event when none appeared.
    """
    reason = capture.detach_reason
    started = time.monotonic()
    give_up = started + REATTACH_SECONDS
    if deadline is not None:
        give_up = min(give_up, deadline)

    out.log(f"[*] gfclient.exe went away ({reason}); waiting up to {REATTACH_SECONDS:g} s for it to come back...")
    out.event("reattaching", detail=reason)

    while True:
        try:
            session = frida.attach("gfclient.exe")
        except Exception:
            session = None

        if session is not None:
            # A detach of the new client while the agent loads sets it again.
            capture.detach_reason = None
            if _attach_script(session, script_code, capture, out):
                out.log(f"[+] Attached to the new gfclient.exe process (PID: {session.pid})")
                out.event("attached", pid=session.pid)
                _announce_waiting(out)
                return session
            reason = capture.detach_reason or reason

        remaining = give_up - time.monotonic()
        if remaining <= 0:
            break
        time.sleep(min(REATTACH_POLL_SECONDS, remaining))

    capture.detach_reason = reason
    waited = time.monotonic() - started
    out.error(f"Detached from gfclient.exe ({reason}) and no new client appeared in {waited:.0f} s")
    return None


def _run(args: argparse.Namespace, out: Output) -> int:
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
    if _attach_script(session, script_code, capture, out):
        _announce_waiting(out)
    elif capture.detach_reason is None:
        return EXIT_FAILED
    # Otherwise the client went away while loading: the loop below re-attaches.

    # Poll an event instead of blocking on stdin: a GUI running the helper
    # through a pipe may never close stdin, and short waits keep Ctrl+C working.
    deadline = None if args.timeout is None else time.monotonic() + args.timeout
    try:
        while True:
            if args.once and capture.count > 0:
                break
            if capture.detach_reason is not None:
                new_session = _reattach(capture, script_code, out, deadline)
                if new_session is None:
                    break
                session = new_session
                continue
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


def main() -> int:
    _make_streams_safe()
    args = parse_args()
    out = Output(args.json)

    try:
        return _run(args, out)
    except Exception as e:
        # Never die with only a traceback: a program running the helper reads
        # the error event; the traceback stays on stderr for bug reports.
        traceback.print_exc(file=sys.stderr)
        out.error(f"unexpected error: {type(e).__name__}: {e}")
        return EXIT_FAILED


if __name__ == "__main__":
    sys.exit(main())
