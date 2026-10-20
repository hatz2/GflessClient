// Node test of the request reassembly (no Frida needed):
//   npm test
import assert from "node:assert/strict";
import { RequestCollector, contentLength } from "../build/test/collector.js";

const body = JSON.stringify({ blackbox: "tra:abc", gsid: "1111-2222", platformGameAccountId: "pga" });
const headers = (len) =>
  `POST /api/v1/auth/thin/codes HTTP/1.1\r\nHost: spark.gameforge.com\r\nContent-Type: application/json\r\nContent-Length: ${len}\r\n\r\n`;

function run(name, fn) {
  const lines = [];
  fn(new RequestCollector((l) => lines.push(l)), lines);
  console.log(`ok - ${name}`);
}

run("headers and body in one write (old client)", (c) => {
  const request = c.feed(headers(body.length) + body);
  assert.equal(request.blackbox, "tra:abc");
});

run("headers first, body in the next write (client 3.0.0.2198)", (c, lines) => {
  assert.equal(c.feed(headers(body.length)), null);
  assert.ok(lines.some((l) => l.includes("Body not in this write yet")));
  const request = c.feed(body);
  assert.equal(request.gsid, "1111-2222");
});

run("body split over several writes, then an unrelated write", (c) => {
  assert.equal(c.feed(headers(body.length)), null);
  assert.equal(c.feed(body.slice(0, 10)), null);
  assert.equal(c.feed(body.slice(10, 30)), null);
  const request = c.feed(body.slice(30));
  assert.equal(request.platformGameAccountId, "pga");
  assert.equal(c.feed("GET /other HTTP/1.1\r\n\r\n"), null);
});

run("other requests are ignored and a body without blackbox is reported", (c, lines) => {
  assert.equal(c.feed("POST /api/v1/auth/iovation HTTP/1.1\r\n\r\n{}"), null);
  const other = JSON.stringify({ gsid: "x", platformGameAccountId: "y" });
  const request = c.feed(headers(other.length) + other);
  assert.equal(request.gsid, "x");
  assert.ok(lines.some((l) => l.includes("No 'blackbox' field")));
});

run("headers cut mid-way and no Content-Length: collects until the JSON parses", (c) => {
  assert.equal(c.feed("POST /api/v1/auth/thin/codes HTTP/1.1\r\nHost: x"), null);
  assert.equal(c.feed("\r\nContent-Type: application/json\r\n\r\n" + body.slice(0, 5)), null);
  const request = c.feed(body.slice(5));
  assert.equal(request.blackbox, "tra:abc");
});

run("a body that never completes is dropped after the write limit", (c, lines) => {
  const small = new RequestCollector((l) => lines.push(l), 3);
  assert.equal(small.feed(headers(500)), null);
  assert.equal(small.feed("{\"a\":"), null);
  assert.equal(small.feed("1"), null);
  assert.equal(small.feed(","), null);
  assert.ok(lines.some((l) => l.includes("Gave up")));
  assert.equal(small.feed(headers(body.length) + body).blackbox, "tra:abc");   // back to normal
});

run("content length parsing", () => {
  assert.equal(contentLength("Content-Length: 42\r\nX: y"), 42);
  assert.equal(contentLength("content-length:7"), 7);
  assert.equal(contentLength("Host: x"), -1);
});

console.log("all collector tests passed");
