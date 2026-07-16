import frida
from frida.core import Session
import sys
import os
import argparse
import subprocess
import base64
import hashlib

def xorBlackbox(data: bytes, key: bytes) -> bytes:
    if not key:
        raise ValueError("Key cannot be empty")

    result = bytearray()

    key_len = len(key)

    for i, b in enumerate(data):
        key_index = i % key_len
        result.append(
            b ^ key[key_index] ^ key[key_len - key_index - 1]
        )

    return bytes(result)


def copy_to_clipboard(text: str) -> None:
    try:
        subprocess.run("clip", input=text, text=True, check=False)
        print("[✅] Copied to clipboard\n")
    except Exception as e:
        print(f"[-] Error copying to clipboard: {e}")


def on_message(message, data):
    if message["type"] == "error":
        print(f"[-] Error: {message['description']}")
        if message.get("stack"):
            print(f"    Stack: {message['stack']}")
        return

    if message["type"] != "send":
        return

    payload: dict = message["payload"]
    request: dict = payload["request"]
    blackbox: str = request["blackbox"]
    blackbox_bytes: bytes = base64.b64decode(blackbox.encode())
    key: bytes = f"{request["gsid"]}-{request["platformGameAccountId"]}".encode()
    key = hashlib.sha512(key).hexdigest().encode()
    blackbox = xorBlackbox(blackbox_bytes, key).decode()
    print(f"\n[🔑] Blackbox: {blackbox}\n")
    copy_to_clipboard(blackbox)


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


def try_attach() -> Session | None:
    try:
        session = frida.attach("gfclient.exe")
        print(f"[+] Attached to existing gfclient.exe process")
        return session
    except frida.ProcessNotFoundError:
        return None
    except Exception as e:
        print(f"[-] Error attaching: {e}")
        return None


def try_spawn(gfclient_path: str | None) -> tuple[Session, int] | tuple[None, None]:
    if not gfclient_path or not os.path.exists(gfclient_path):
        print(f"[-] gfclient.exe not found at: {gfclient_path}")
        return None, None

    try:
        print(f"[*] Spawning {gfclient_path}...")
        pid = frida.spawn(gfclient_path)
        session = frida.attach(pid)
        frida.resume(pid)
        print(f"[+] Spawned and attached to PID: {pid}")
        return session, pid
    except Exception as e:
        print(f"[-] Error spawning: {e}")
        return None, None


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


def main():
    parser = argparse.ArgumentParser(
        prog="BlackboxExtractor",
        description="Extracts blackbox field from auth/iovation requests in gfclient.exe"
    )
    parser.add_argument(
        "gfclient_path",
        nargs="?",
        default=None,
        help="Path to gfclient.exe (optional, auto-detected if not provided)"
    )
    parser.add_argument(
        "--spawn", action="store_true",
        help="Force spawn gfclient.exe instead of attaching"
    )
    args = parser.parse_args()

    script_path = _find_script()
    if not script_path:
        print("[-] Script file 'build/agent.js' not found")
        print("    Run 'npm run build' first")
        return

    with open(script_path, "r", encoding="utf-8") as f:
        script_code = f.read()

    gfclient_path = find_gfclient(args.gfclient_path)

    session = None

    if not args.spawn:
        session = try_attach()

    if not session:
        session, _ = try_spawn(gfclient_path)
        if not session:
            print("[-] Could not attach to or spawn gfclient.exe")
            return

    script = session.create_script(script_code)
    script.on("message", on_message)

    try:
        script.load()
    except Exception as e:
        print(f"[-] Error loading script: {e}")
        session.detach()
        return

    print(f"[+] Script loaded successfully")
    print(f"[*] Waiting for auth/iovation requests...")
    print(f"[*] Press Ctrl+C to exit\n")

    try:
        sys.stdin.read()
    except KeyboardInterrupt:
        print("\n[*] Detaching...")
    finally:
        session.detach()


if __name__ == "__main__":
    main()
