"""Unit tests of the attach / wait / re-attach loop of client.py.

A fake frida module replaces the real one: no Frida, game client or process
is needed. Run from the BlackboxExtractor folder:
    .venv\\Scripts\\python.exe -m unittest discover -s tests -p "test_*.py"
"""

import io
import json
import os
import sys
import threading
import time
import types
import unittest
from unittest import mock

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src"))


class ProcessNotFoundError(Exception):
    pass


class FakeScript:
    def __init__(self, on_load=None) -> None:
        self.handlers = {}
        self.on_load = on_load

    def set_log_handler(self, handler) -> None:
        self.log_handler = handler

    def on(self, signal: str, callback) -> None:
        self.handlers[signal] = callback

    def load(self) -> None:
        if self.on_load is not None:
            self.on_load(self)

    def send_blackbox(self, blackbox: str = "tra:FAKE") -> None:
        """What the agent posts for a captured auth request."""
        message = {"type": "send", "payload": {"path": "/api/v1/auth/iovation", "request": {"blackbox": blackbox}}}
        self.handlers["message"](message, None)


class FakeSession:
    def __init__(self, pid: int, on_load=None, create_error: Exception | None = None, gone: bool = False) -> None:
        self.pid = pid
        self.on_load = on_load
        self.create_error = create_error
        self.is_detached = gone
        self.detached_handlers = []
        self.script = None

    def on(self, signal: str, callback) -> None:
        assert signal == "detached"
        self.detached_handlers.append(callback)

    def off(self, signal: str, callback) -> None:
        self.detached_handlers.remove(callback)

    def create_script(self, code: str) -> FakeScript:
        if self.create_error is not None:
            raise self.create_error
        self.script = FakeScript(self.on_load)
        return self.script

    def detach(self) -> None:
        self.terminate("application-requested")

    def terminate(self, reason: str = "process-terminated") -> None:
        """The process went away: frida reports it to every detached handler."""
        if self.is_detached:
            return
        self.is_detached = True
        for handler in list(self.detached_handlers):
            handler(reason, None)


class FakeFrida(types.ModuleType):
    """frida.attach returns the queued results in order (a session or an exception)."""

    ProcessNotFoundError = ProcessNotFoundError

    def __init__(self, attach_results=(), spawn_session: FakeSession | None = None) -> None:
        super().__init__("frida")
        self.attach_results = list(attach_results)
        self.spawn_session = spawn_session
        self.attach_calls = 0
        self.spawn_calls = 0

    def attach(self, target):
        self.attach_calls += 1
        if isinstance(target, int):
            # Attach to the process spawned just before.
            return self.spawn_session
        if not self.attach_results:
            raise ProcessNotFoundError("unable to find process with name 'gfclient.exe'")
        result = self.attach_results.pop(0)
        if isinstance(result, Exception):
            raise result
        return result

    def spawn(self, path):
        self.spawn_calls += 1
        return self.spawn_session.pid

    def resume(self, pid) -> None:
        pass


# client.py imports frida (and frida.core) at module level.
_stub = FakeFrida()
_stub_core = types.ModuleType("frida.core")
_stub_core.Session = FakeSession
_stub.core = _stub_core
sys.modules.setdefault("frida", _stub)
sys.modules.setdefault("frida.core", _stub_core)

import client  # noqa: E402


def send_blackbox_soon(blackbox: str = "tra:FAKE"):
    """on_load hook: the agent posts a captured blackbox from frida's thread."""

    def on_load(script: FakeScript) -> None:
        timer = threading.Timer(0.05, script.send_blackbox, args=(blackbox,))
        timer.daemon = True
        timer.start()

    return on_load


class ClientLoopTest(unittest.TestCase):
    def setUp(self) -> None:
        # Keep every run short: no 0.25 s / 2 s waits.
        for name, value in (("POLL_SECONDS", 0.01), ("REATTACH_POLL_SECONDS", 0.02)):
            patcher = mock.patch.object(client, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)

    def run_main(self, fake: FakeFrida, *args: str) -> tuple[int, list[dict], str]:
        """Run client.main() in --json mode; return (exit code, events, stderr)."""
        stdout, stderr = io.StringIO(), io.StringIO()
        argv = ["BlackboxExtractor", "--json", *args]
        # Any readable file stands in for build/agent.js (no build needed).
        with mock.patch.object(client, "frida", fake), \
                mock.patch.object(client, "_find_script", return_value=os.path.abspath(__file__)), \
                mock.patch.object(sys, "argv", argv), \
                mock.patch.object(sys, "stdout", stdout), \
                mock.patch.object(sys, "stderr", stderr):
            code = client.main()
        events = [json.loads(line) for line in stdout.getvalue().splitlines() if line.strip()]
        return code, events, stderr.getvalue()

    def names(self, events: list[dict]) -> list[str]:
        return [event["event"] for event in events]

    def test_crash_in_create_script_is_an_error_event_and_exit_2(self):
        session = FakeSession(100, create_error=RuntimeError("boom"))
        code, events, stderr = self.run_main(FakeFrida([session]), "--no-spawn", "--timeout", "5")

        self.assertEqual(code, client.EXIT_FAILED)
        self.assertEqual(self.names(events), ["attached", "error"])
        self.assertIn("could not load the agent into gfclient.exe (RuntimeError: boom)", events[-1]["detail"])
        self.assertNotIn("Traceback", stderr)
        self.assertTrue(session.is_detached)

    def test_detach_then_new_client_is_reattached_and_captured(self):
        first = FakeSession(100, on_load=lambda script: first.terminate("process-terminated"))
        second = FakeSession(200, on_load=send_blackbox_soon("tra:SECOND"))
        # The first poll after the detach finds nothing (the client is restarting).
        fake = FakeFrida([first, ProcessNotFoundError("not yet"), second])

        started = time.monotonic()
        code, events, _ = self.run_main(fake, "--no-spawn", "--once", "--timeout", "5")

        self.assertEqual(code, client.EXIT_CAPTURED)
        self.assertEqual(
            self.names(events),
            ["attached", "waiting", "reattaching", "attached", "waiting", "blackbox"],
        )
        self.assertEqual(events[0]["pid"], 100)
        self.assertEqual(events[2]["detail"], "process-terminated")
        self.assertEqual(events[3]["pid"], 200)
        self.assertEqual(events[-1]["blackbox"], "tra:SECOND")
        self.assertEqual(fake.spawn_calls, 0)
        self.assertLess(time.monotonic() - started, 2)

    def test_detach_and_no_new_client_is_an_error_event_and_exit_1(self):
        first = FakeSession(100, on_load=lambda script: first.terminate("process-terminated"))
        fake = FakeFrida([first])

        started = time.monotonic()
        with mock.patch.object(client, "REATTACH_SECONDS", 0.3):
            code, events, _ = self.run_main(fake, "--no-spawn", "--timeout", "5")
        elapsed = time.monotonic() - started

        self.assertEqual(code, client.EXIT_NOTHING)
        self.assertEqual(self.names(events), ["attached", "waiting", "reattaching", "error"])
        self.assertIn("Detached from gfclient.exe (process-terminated)", events[-1]["detail"])
        self.assertIn("no new client appeared", events[-1]["detail"])
        self.assertGreaterEqual(elapsed, 0.3)
        self.assertLess(elapsed, 2)
        # Polled again and again during the budget, never spawned.
        self.assertGreater(fake.attach_calls, 3)
        self.assertEqual(fake.spawn_calls, 0)

    def test_reattach_budget_stays_inside_the_timeout(self):
        first = FakeSession(100, on_load=lambda script: first.terminate("process-terminated"))

        started = time.monotonic()
        code, events, _ = self.run_main(FakeFrida([first]), "--no-spawn", "--timeout", "0.3")

        self.assertEqual(code, client.EXIT_NOTHING)
        self.assertEqual(self.names(events)[-1], "error")
        self.assertLess(time.monotonic() - started, 2)

    def test_spawned_client_that_restarts_while_loading_is_reattached(self):
        # The freshly spawned client re-executes itself: the session is gone
        # before the agent could be created.
        spawned = FakeSession(100, create_error=RuntimeError("session is gone"), gone=True)
        restarted = FakeSession(200, on_load=send_blackbox_soon())
        fake = FakeFrida([restarted], spawn_session=spawned)

        code, events, _ = self.run_main(fake, "--spawn", "--once", "--timeout", "5", os.path.abspath(__file__))

        self.assertEqual(code, client.EXIT_CAPTURED)
        self.assertEqual(self.names(events), ["spawned", "reattaching", "attached", "waiting", "blackbox"])
        self.assertEqual(fake.spawn_calls, 1)

    def test_unexpected_exception_is_an_error_event_and_exit_2(self):
        session = FakeSession(100)
        with mock.patch.object(client, "_attach_script", side_effect=ValueError("kaput")):
            code, events, stderr = self.run_main(FakeFrida([session]), "--no-spawn", "--timeout", "5")

        self.assertEqual(code, client.EXIT_FAILED)
        self.assertEqual(self.names(events), ["attached", "error"])
        self.assertEqual(events[-1]["detail"], "unexpected error: ValueError: kaput")
        self.assertIn("Traceback", stderr)

    def test_no_running_client_with_no_spawn_is_exit_2(self):
        code, events, _ = self.run_main(FakeFrida([]), "--no-spawn", "--timeout", "5")

        self.assertEqual(code, client.EXIT_FAILED)
        self.assertEqual(self.names(events), ["error"])
        self.assertIn("gfclient.exe is not running", events[0]["detail"])


if __name__ == "__main__":
    unittest.main()
