import { RequestCollector } from "./collector.js";

// OpenSSL libraries gfclient.exe has shipped with, newest first. Since the
// 2026-10-13 client update (3.0.0.2198, Qt 5.12.5) Qt5Network loads the
// OpenSSL 1.1 pair (libssl-1_1.dll / libcrypto-1_1.dll); the old
// ssleay32.dll / libeay32.dll files are still in the folder but never loaded.
const SSL_MODULE_CANDIDATES = ["libssl-1_1.dll", "libssl-3.dll", "ssleay32.dll"];

// Qt loads OpenSSL lazily, on the first HTTPS request, so right after a spawn
// (or a very early attach) none of the candidates is in the process yet.
const MODULE_POLL_MS = 500;
const MODULE_POLL_LIMIT = 240; // 2 minutes

class BlackboxExtractor {
  private hooked = false;
  private polls = 0;
  private readonly collector = new RequestCollector((line) => console.log(line));

  constructor() {
    if (!this.hookSSL()) {
      console.log(`[*] No OpenSSL module loaded yet (looked for ${SSL_MODULE_CANDIDATES.join(", ")}); waiting for it...`);
      const timer = setInterval(() => {
        this.polls += 1;
        if (this.hookSSL()) {
          clearInterval(timer);
        } else if (this.polls >= MODULE_POLL_LIMIT) {
          clearInterval(timer);
          console.log("[-] Gave up: no OpenSSL module appeared in gfclient.exe. Loaded modules:");
          for (const m of Process.enumerateModules()) {
            if (/ssl|eay|crypto|qt5network/i.test(m.name)) {
              console.log(`    ${m.name} at ${m.base}`);
            }
          }
        }
      }, MODULE_POLL_MS);
    }
  }

  private hookSSL(): boolean {
    if (this.hooked) {
      return true;
    }
    for (const modName of SSL_MODULE_CANDIDATES) {
      const mod = Process.findModuleByName(modName);
      if (!mod) {
        continue;
      }
      console.log(`[+] Found module: ${modName} at ${mod.base}`);
      const sslWriteAddr = this.findExport(mod, "SSL_write");
      if (!sslWriteAddr) {
        console.log(`[-] ${modName} has no SSL_write export`);
        continue;
      }
      console.log(`[+] SSL_write at ${sslWriteAddr}`);
      Interceptor.attach(sslWriteAddr, {
        onEnter: (args) => this.onSSLWrite(args),
      });
      this.hooked = true;
      return true;
    }
    return false;
  }

  private onSSLWrite(args: InvocationArguments): void {
    const buf = args[1];
    const num = args[2].toInt32();
    if (num <= 0) {
      return;
    }
    let data: string | null;
    try {
      data = buf.readUtf8String(num);
    } catch (e) {
      return;
    }
    if (!data) {
      return;
    }
    try {
      const request = this.collector.feed(data);
      if (request !== null) {
        send({ request: request });
      }
    } catch (e) {
      console.log(`[-] Error while reading the request: ${e}`);
    }
  }

  private findExport(
    mod: Module,
    name: string,
  ): NativePointer | null | undefined {
    const exports = mod.enumerateExports();
    for (const exp of exports) {
      if (exp.name === name) {
        return exp.address;
      }
    }
    return null;
  }
}

const extractor = new BlackboxExtractor();
