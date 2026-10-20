// Reassembles Gameforge auth requests (POST /api/v1/auth/<endpoint>: the
// client login `sessions`, `iovation` and `thin/codes`) from what the client
// hands to SSL_write.
//
// The old Gameforge client wrote the HTTP headers and the JSON body of the
// POST in one SSL_write; since the 2026-10-13 update (Qt 5.12.5) they arrive
// in separate writes, and a body may even be split. This class only knows
// strings, so it can be tested with plain Node.

export type RequestLog = (line: string) => void;

const IDLE = -1;
const HEADERS_PENDING = -2;
const UNKNOWN_LENGTH = Number.MAX_SAFE_INTEGER;

// The request line is always at the start of the first write of a request.
// The captured path leaves out any query string.
const AUTH_REQUEST_LINE = /^POST (\/api\/v1\/auth\/[^\s?]+)\S* HTTP\//;

/** A complete auth request: its path and its parsed JSON body. */
export interface AuthRequest {
  path: string;
  body: any;
}

export class RequestCollector {
  private pendingLength = IDLE;
  private pendingBody = "";
  private pendingWrites = 0;
  private pendingPath = "";

  constructor(
    private readonly log: RequestLog,
    private readonly bodyWaitLimit = 16,
  ) {}

  /** Feed one SSL_write payload; returns the path and the parsed JSON body when an auth request is complete. */
  feed(data: string): AuthRequest | null {
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

    const requestLine = AUTH_REQUEST_LINE.exec(data);
    if (requestLine === null) {
      return null;
    }
    this.pendingPath = requestLine[1];
    this.log(`[→] ${this.pendingPath} detected in SSL_write!`);
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
  private startBody(length: number, body: string): AuthRequest | null {
    this.pendingLength = length > 0 ? length : UNKNOWN_LENGTH;
    this.pendingBody = body;
    this.pendingWrites = 0;
    return this.bodyProgress(true);
  }

  private bodyProgress(first = false): AuthRequest | null {
    if (this.pendingLength === UNKNOWN_LENGTH) {
      // No Content-Length: the body is complete when it parses.
      try {
        const request = JSON.parse(this.pendingBody);
        const path = this.pendingPath;
        this.reset();
        return this.report(path, request);
      } catch (e) {
        return this.giveUpIfStuck();
      }
    }
    if (this.pendingBody.length >= this.pendingLength) {
      const body = this.pendingBody.substring(0, this.pendingLength);
      const path = this.pendingPath;
      this.reset();
      try {
        return this.report(path, JSON.parse(body));
      } catch (e) {
        // No content in the log: a login body carries the account password.
        this.log(`[-] ${path} request body is not JSON (${body.length} bytes)`);
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

  private report(path: string, body: any): AuthRequest {
    const fields = body !== null && typeof body === "object" ? Object.keys(body) : [];
    this.log(`[+] ${path} request fields: ${fields.join(", ")}`);
    if (!fields.includes("blackbox")) {
      this.log("[-] No 'blackbox' field in the request; the format changed, see the fields above");
    }
    return { path, body };
  }

  private reset(): void {
    this.pendingLength = IDLE;
    this.pendingBody = "";
    this.pendingWrites = 0;
    this.pendingPath = "";
  }
}

export function contentLength(headers: string): number {
  const match = /content-length:\s*(\d+)/i.exec(headers);
  return match ? parseInt(match[1], 10) : -1;
}
