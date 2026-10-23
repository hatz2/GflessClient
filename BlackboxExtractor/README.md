# BlackboxExtractor

Extracts the `blackbox` field from auth/iovation HTTPS requests in `gfclient.exe` using Frida.

Hooks `SSL_write` in the OpenSSL library the client loads (`libssl-1_1.dll` since the 3.0.0.2198 client of 2026-10,
`ssleay32.dll` before) to capture decrypted traffic, reassembles the JSON body of the `thin/codes` request
(the current client sends headers and body in separate writes), and copies the blackbox value to clipboard.

## Prerequisites

- [Node.js](https://nodejs.org/) (for compiling the Frida agent)
- [Python](https://python.org/) 3.8+
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

## Tests

```bash
npm test   # the request reassembly, no Frida or game client needed
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
