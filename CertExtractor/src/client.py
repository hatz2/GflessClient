import frida
import sys
import os
import argparse

def on_message(message, data):
    if (message["type"] == "error"):
        print(f"[-] Error: {message}")
        return
    
    if (message["type"] != "send"):
        print(f"[-] Unknown: {message}")
        return
    
    pass_phrase = message["payload"]["passPhrase"]

    with open('client.p12', 'wb') as f:
        f.write(data)

    print(f"\n{'='*60}")
    print(f"[+] Certificate dumped successfully")
    print(f"[+] Certificate pass phrase: {pass_phrase}")

def main(gfclient_path: str) -> None:
    SCRIPT_FILE = "build/agent.js"
    
    # Script check
    if not os.path.exists(SCRIPT_FILE):
        print(f"[-] Error: Script file '{SCRIPT_FILE}' not found")
        return
    
    # Script load
    with open(SCRIPT_FILE, 'r', encoding='utf-8') as f:
        script_code = f.read()
    
    # Spawn gfclient
    print(f"[*] Spawning gfclient...")
    
    if not os.path.exists(gfclient_path):
        print(f"[-] Error: Executable not found at {gfclient_path}")
        return
    
    pid = frida.spawn(gfclient_path)
    session = frida.attach(pid)
    print(f"[+] Spawned and attached to PID: {pid}")
    frida.resume(pid)
    print(f"[+] Resumed process")
    
    # Create and load the script
    script = session.create_script(script_code)
    script.on('message', on_message)
    script.load()
    
    print(f"[+] Script loaded successfully")
    print(f"[*] Press Ctrl+C to exit\n")
    
    try:
        sys.stdin.read()
    except KeyboardInterrupt:
        print("\n[*] Detaching...")
        session.detach()
        print("[*] Done.")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='Certificate extractor',
        description='Exctracts the embedded SSL certificate from gfclient'
    )
    parser.add_argument("gfclient_path")
    args = parser.parse_args()
    gfclient_path = args.gfclient_path
    main(gfclient_path)