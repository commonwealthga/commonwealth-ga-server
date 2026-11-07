#!/usr/bin/env python3
"""Minimal ServiceHooks service: answers every event with "pass" (= the
control server uses its built-in rule). Copy it as a starting point.

Config in control-server.json:
  "services": {"example": {"addr": "127.0.0.1:7700", "timeout_ms": 200,
                            "events": ["level_up"]}}
Frame: 4-byte little-endian length + UTF-8 JSON, one request, one reply.
"""
import json
import socketserver
import struct
import sys


def recv_exact(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("closed")
        buf += chunk
    return buf


def decide(event, data):
    # Return a dict to make a decision, or None to pass.
    return None


class Handler(socketserver.BaseRequestHandler):
    def handle(self):
        try:
            (n,) = struct.unpack("<I", recv_exact(self.request, 4))
            if n == 0 or n > 65536:
                return
            req = json.loads(recv_exact(self.request, n))
            print(f"{req.get('event')} {json.dumps(req.get('data'))}", flush=True)
            decision = decide(req.get("event"), req.get("data") or {})
            reply = {"v": 1, "id": req.get("id")}
            if decision is None:
                reply["pass"] = True
            else:
                reply["decision"] = decision
            out = json.dumps(reply).encode()
            self.request.sendall(struct.pack("<I", len(out)) + out)
        except (ConnectionError, ValueError, struct.error):
            pass


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 7700
    socketserver.ThreadingTCPServer.allow_reuse_address = True
    with socketserver.ThreadingTCPServer(("127.0.0.1", port), Handler) as srv:
        srv.serve_forever()
