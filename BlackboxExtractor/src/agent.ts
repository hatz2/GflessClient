class BlackboxExtractor {
  constructor() {
    this.hookSSL();
  }

  private hookSSL(): void {
    const modName = "ssleay32.dll";
    const mod = Process.findModuleByName(modName);
    if (!mod) {
      console.log(`[-] Could not find module ${modName}`);
      return;
    }

    console.log(`[+] Found module: ${modName} at ${mod.base}`);

    const sslWriteAddr = this.findExport(mod, "SSL_write");

    if (sslWriteAddr) {
      console.log(`[+] SSL_write at ${sslWriteAddr}`);
    } else {
      console.log("[-] Could not find SSL_write");
      return;
    }

    Interceptor.attach(sslWriteAddr, {
      onEnter: (args) => this.onSSLWrite(args),
    });
  }

  private onSSLWrite(args: InvocationArguments): void {
    const buf = args[1];
    const num = args[2].toInt32();

    try {
      const data = buf.readUtf8String(num);
      if (!data) {
        return;
      }

      if (data.includes("thin/codes")) {
        console.log("[→] /api/v1/auth/thin/codes detected in SSL_write!");
        const body = this.extractBody(data);

        if (body === null) {
          console.log("[-] Could not extract body from request");
          return;
        }

        const request = JSON.parse(body);
        send({ request: request });
      }
    } catch (e) {}
  }

  private extractBody(data: string): string | null {
    const bodyStart = data.indexOf("\r\n\r\n");
    return bodyStart !== -1 ? data.substring(bodyStart + 4) : null;
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
