// Reassembles the thin/codes request from what the client hands to SSL_write.
//
// The old Gameforge client wrote the HTTP headers and the JSON body of the
// POST in one SSL_write; since the 2026-10-13 update (Qt 5.12.5) they arrive
// in separate writes, and a body may even be split. This class only knows
// strings, so it can be tested with plain Node.

export type RequestLog = (line: string) => void;

const IDLE = -1;
const HEADERS_PENDING = -2;
const UNKNOWN_LENGTH = Number.MAX_SAFE_INTEGER;

export class RequestCollector {
  private pendingLength = IDLE;
  private pendingBody = "";
  private pendingWrites = 0;

  constructor(
    private readonly log: RequestLog,
    private readonly bodyWaitLimit = 16,
  ) {}

  /** Feed one SSL_write payload; returns the parsed JSON body when a thin/codes request is complete. */
  feed(data: string): any | null {
    if (this.pendingLength === HEADERS_PENDING) {
      // The headers were cut mid-way: wait for their end, then for the body.
      this.pendingBody += data;
      this.pendingWrites += 1;
      const headerEnd = this.pendingBody.indexOf("\r\n\r\n");
      if (headerEnd === -1) {
        return this.giveUpIfStuck();
      }
      const headers = this.pendingBody.substring(0, headerEnd);
      const body = this.pendingBody.substring(headerEnd + 4);
      return this.startBody(contentLength(headers), body);
    }

    if (this.pendingLength >= 0) {
      this.pendingBody += data;
      this.pendingWrites += 1;
      return this.bodyProgress();
    }

    if (!data.includes("thin/codes")) {
      return null;
    }
    this.log("[→] /api/v1/auth/thin/codes detected in SSL_write!");
    const headerEnd = data.indexOf("\r\n\r\n");
    if (headerEnd === -1) {
      this.log("[-] Request headers are incomplete; waiting for the rest");
      this.pendingLength = HEADERS_PENDING;
      this.pendingBody = data;
      this.pendingWrites = 0;
      return null;
    }
    return this.startBody(contentLength(data.substring(0, headerEnd)), data.substring(headerEnd + 4));
  }

  /** The headers are complete: `body` is what came with them so far. */
  private startBody(length: number, body: string): any | null {
    this.pendingLength = length > 0 ? length : UNKNOWN_LENGTH;
    this.pendingBody = body;
    this.pendingWrites = 0;
    return this.bodyProgress(true);
  }

  private bodyProgress(first = false): any | null {
    if (this.pendingLength === UNKNOWN_LENGTH) {
      // No Content-Length: the body is complete when it parses.
      try {
        const request = JSON.parse(this.pendingBody);
        this.reset();
        return this.report(request);
      } catch (e) {
        return this.giveUpIfStuck();
      }
    }
    if (this.pendingBody.length >= this.pendingLength) {
      const body = this.pendingBody.substring(0, this.pendingLength);
      this.reset();
      try {
        return this.report(JSON.parse(body));
      } catch (e) {
        this.log(`[-] Request body is not JSON (${body.length} bytes): ${body.substring(0, 80)}`);
        return null;
      }
    }
    if (first) {
      this.log(`[*] Body not in this write yet (${this.pendingBody.length} of ${this.pendingLength} bytes); collecting the next writes`);
    }
    return this.giveUpIfStuck();
  }

  private giveUpIfStuck(): null {
    if (this.pendingWrites >= this.bodyWaitLimit) {
      this.log(`[-] Gave up collecting the request body after ${this.pendingWrites} writes (${this.pendingBody.length} bytes)`);
      this.reset();
    }
    return null;
  }

  private report(request: any): any {
    this.log(`[+] Request fields: ${Object.keys(request).join(", ")}`);
    if (!("blackbox" in request)) {
      this.log("[-] No 'blackbox' field in the request; the format changed, see the fields above");
    }
    return request;
  }

  private reset(): void {
    this.pendingLength = IDLE;
    this.pendingBody = "";
    this.pendingWrites = 0;
  }
}

export function contentLength(headers: string): number {
  const match = /content-length:\s*(\d+)/i.exec(headers);
  return match ? parseInt(match[1], 10) : -1;
}
