# BlackboxExtractor

Extracts the `blackbox` field from the auth HTTPS requests of `gfclient.exe` using Frida.

Hooks `SSL_write` in the OpenSSL library the client loads (`libssl-1_1.dll` since the 3.0.0.2198 client of 2026-10,
`ssleay32.dll` before) to capture decrypted traffic and reassembles the JSON body of every
`POST /api/v1/auth/<endpoint>` request (the current client sends headers and body in separate writes). The blackbox
is taken from any of them:

- `auth/iovation`: the device check (plain `tra:...` value),
- `auth/thin/codes`: the game code request (base64, XORed with a key derived from `gsid` and
  `platformGameAccountId`; decoded by the host).

Both are sent when **Play** is pressed in the Gameforge client. The client's own login runs in its embedded
browser, which has its own TLS library, so it is not seen here. Any `password` field of a captured request is
dropped inside the target process and never reaches the host.

## Prerequisites

- [Node.js](https://nodejs.org/) (for compiling the Frida agent)
- [Python](https://python.org/) 3.10+
- [Frida](https://frida.re/) (`pip install frida`)

## Quick Start (development)

```bash
# 1. Install dependencies and compile the Frida agent
npm install
npm run build

# 2. Attach to an already running gfclient.exe
python src/client.py

# Or spawn gfclient.exe from scratch
python src/client.py --spawn

# Or specify a custom path
python src/client.py "C:/path/to/gfclient.exe"
```

Without options the extractor attaches to `gfclient.exe` (or starts it when it is not running), prints every captured
blackbox, copies it to the clipboard and keeps waiting until Ctrl+C.

## Options

| Option | Effect |
| --- | --- |
| `gfclient_path` | Path to `gfclient.exe` used when it has to be started (auto-detected when omitted). |
| `--spawn` | Always start a new `gfclient.exe` instead of attaching to a running one. |
| `--no-spawn` | Attach only; when `gfclient.exe` is not running, exit with code 2 instead of starting it. |
| `--json` | Machine-readable mode: stdout carries only JSON events, one object per line, flushed immediately; every log line goes to stderr. Nothing is copied to the clipboard. |
| `--once` | Detach and exit as soon as the first blackbox was captured. |
| `--timeout SECONDS` | Exit with code 1 when nothing was captured within that time (default: wait forever). |

Example for a program that runs the extractor as a helper:

```bash
BlackboxExtractor.exe --json --once --timeout 600
```

### JSON events (`--json`)

```json
{"event": "attached", "pid": 1234}
{"event": "spawned", "pid": 1234}
{"event": "waiting"}
{"event": "reattaching", "detail": "process-terminated"}
{"event": "blackbox", "blackbox": "tra:...", "path": "/api/v1/auth/sessions"}
{"event": "error", "detail": "..."}
```

`attached` or `spawned` comes first, then `waiting` once the hook is loaded. An `error` event is printed for every
failure that ends the run with code 2 (an unexpected internal error included), when `gfclient.exe` closed and no new
one appeared, and for a request whose blackbox could not be decoded (the extractor keeps waiting in that case).

The Gameforge client may update or restart itself when it starts; the extractor waits for it. When `gfclient.exe` goes
away, `reattaching` is printed with the reason, and for up to 90 s (never past `--timeout`) the extractor looks for a
new `gfclient.exe` every 2 s. Once it is found the hook is loaded again and `attached` and `waiting` follow. Only when
no new client appears in time does the run end with an `error` event and code 1. Re-attaching never starts a client,
also without `--no-spawn`.

### Exit codes

| Code | Meaning |
| --- | --- |
| 0 | At least one blackbox was captured. |
| 1 | Nothing captured (timeout, Ctrl+C, or `gfclient.exe` closed and no new one appeared within 90 s). |
| 2 | Could not attach to or start `gfclient.exe`, could not load the agent script, or an unexpected error (its traceback goes to stderr). |

## Tests

No Frida or game client needed:

```bash
npm test         # the request reassembly (src/collector.ts)
npm run test:py  # the blackbox decoding and the attach / re-attach loop (src/client.py, fake frida), same as:
.venv\Scripts\python.exe -m unittest discover -s tests -p "test_*.py"
```

## Building a self-contained .exe

Use the provided PowerShell script:

```powershell
.\build_exe.ps1
```

This will:
1. Install npm dependencies and compile the Frida agent
2. Install PyInstaller
3. Package everything into a single `dist/BlackboxExtractor.exe`

### Options

```powershell
.\build_exe.ps1 -Clean           # Clean previous builds first
.\build_exe.ps1 -UseVenb         # Use/create an isolated Python virtual env
.\build_exe.ps1 -Clean -UseVenb  # Combine both
```

### Manual PyInstaller build

```bash
pip install pyinstaller
pyinstaller --onefile --add-data "build/agent.js;build" --name BlackboxExtractor src/client.py
```

Output: `dist/BlackboxExtractor.exe`
