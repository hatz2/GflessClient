// Node test of the request reassembly (no Frida needed):
//   npm test
import assert from "node:assert/strict";
import { RequestCollector, contentLength } from "../build/test/collector.js";

const body = JSON.stringify({ blackbox: "tra:abc", gsid: "1111-2222", platformGameAccountId: "pga" });
const request = (path, len) =>
  `POST ${path} HTTP/1.1\r\nHost: spark.gameforge.com\r\nContent-Type: application/json\r\nContent-Length: ${len}\r\n\r\n`;
const headers = (len) => request("/api/v1/auth/thin/codes", len);

function run(name, fn) {
  const lines = [];
  fn(new RequestCollector((l) => lines.push(l)), lines);
  console.log(`ok - ${name}`);
}

run("headers and body in one write (old client)", (c) => {
  const captured = c.feed(headers(body.length) + body);
  assert.equal(captured.path, "/api/v1/auth/thin/codes");
  assert.equal(captured.body.blackbox, "tra:abc");
});

run("headers first, body in the next write (client 3.0.0.2198)", (c, lines) => {
  assert.equal(c.feed(headers(body.length)), null);
  assert.ok(lines.some((l) => l.includes("Body not in this write yet")));
  const captured = c.feed(body);
  assert.equal(captured.path, "/api/v1/auth/thin/codes");
  assert.equal(captured.body.gsid, "1111-2222");
});

run("body split over several writes, then an unrelated write", (c) => {
  assert.equal(c.feed(headers(body.length)), null);
  assert.equal(c.feed(body.slice(0, 10)), null);
  assert.equal(c.feed(body.slice(10, 30)), null);
  const captured = c.feed(body.slice(30));
  assert.equal(captured.body.platformGameAccountId, "pga");
  assert.equal(c.feed("GET /other HTTP/1.1\r\n\r\n"), null);
});

run("a body without blackbox is still returned and reported", (c, lines) => {
  const other = JSON.stringify({ gsid: "x", platformGameAccountId: "y" });
  const captured = c.feed(headers(other.length) + other);
  assert.equal(captured.body.gsid, "x");
  assert.ok(lines.some((l) => l.includes("No 'blackbox' field")));
});

run("headers cut mid-way and no Content-Length: collects until the JSON parses", (c) => {
  assert.equal(c.feed("POST /api/v1/auth/thin/codes HTTP/1.1\r\nHost: x"), null);
  assert.equal(c.feed("\r\nContent-Type: application/json\r\n\r\n" + body.slice(0, 5)), null);
  const captured = c.feed(body.slice(5));
  assert.equal(captured.path, "/api/v1/auth/thin/codes");
  assert.equal(captured.body.blackbox, "tra:abc");
});

run("a body that never completes is dropped after the write limit", (c, lines) => {
  const small = new RequestCollector((l) => lines.push(l), 3);
  assert.equal(small.feed(headers(500)), null);
  assert.equal(small.feed("{\"a\":"), null);
  assert.equal(small.feed("1"), null);
  assert.equal(small.feed(","), null);
  assert.ok(lines.some((l) => l.includes("Gave up")));
  assert.equal(small.feed(headers(body.length) + body).body.blackbox, "tra:abc");   // back to normal
});

run("content length parsing", () => {
  assert.equal(contentLength("Content-Length: 42\r\nX: y"), 42);
  assert.equal(contentLength("content-length:7"), 7);
  assert.equal(contentLength("Host: x"), -1);
});

run("client login (sessions) with a plain tra: blackbox", (c) => {
  // The login request carries the blackbox in the clear, without gsid.
  const login = JSON.stringify({ identity: "user@example.com", locale: "en_GB", blackbox: "tra:login" });
  assert.equal(c.feed(request("/api/v1/auth/sessions", login.length)), null);
  const captured = c.feed(login);
  assert.equal(captured.path, "/api/v1/auth/sessions");
  assert.equal(captured.body.blackbox, "tra:login");
});

run("iovation request", (c) => {
  const iovation = JSON.stringify({ type: "play_now", blackbox: "tra:iov" });
  const captured = c.feed(request("/api/v1/auth/iovation", iovation.length) + iovation);
  assert.equal(captured.path, "/api/v1/auth/iovation");
  assert.equal(captured.body.blackbox, "tra:iov");
});

run("OPTIONS preflight is ignored", (c, lines) => {
  assert.equal(
    c.feed("OPTIONS /api/v1/auth/sessions HTTP/1.1\r\nHost: spark.gameforge.com\r\nAccess-Control-Request-Method: POST\r\n\r\n"),
    null,
  );
  assert.equal(lines.length, 0);
  // The collector is still idle: a body-like write right after is not taken as a body.
  assert.equal(c.feed(body), null);
});

run("a POST outside /api/v1/auth is ignored", (c, lines) => {
  assert.equal(c.feed(request("/api/v1/users/me/accounts", body.length) + body), null);
  assert.equal(c.feed(request("/api/v2/auth/sessions", body.length) + body), null);
  assert.equal(lines.length, 0);
});

run("the path is reported without the query string", (c, lines) => {
  const captured = c.feed(request("/api/v1/auth/thin/codes?locale=en", body.length) + body);
  assert.equal(captured.path, "/api/v1/auth/thin/codes");
  assert.ok(lines.some((l) => l.includes("/api/v1/auth/thin/codes detected")));
  assert.ok(lines.some((l) => l.includes("/api/v1/auth/thin/codes request fields: blackbox, gsid, platformGameAccountId")));
});

console.log("all collector tests passed");
