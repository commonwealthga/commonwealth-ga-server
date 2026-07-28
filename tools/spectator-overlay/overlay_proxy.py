#!/usr/bin/env python3
"""Thin public-facing proxy for the spectator broadcast overlay.

Serves the two static overlay pages and forwards their /overlay* API calls to
the control server's overlay HTTP endpoint, attaching the secret token
server-side. The token therefore never appears in the pages, in the browser,
or in any public URL — it lives only in the OVERLAY_TOKEN environment
variable (and must match overlay_token in control-server.json).

Environment:
  OVERLAY_UPSTREAM  base URL of the control server's overlay listener
                    (default http://127.0.0.1:8090). With the docker-compose
                    setup this is http://host.docker.internal:8090 — note a
                    control server bound to 127.0.0.1 is NOT reachable from
                    inside the container, so either set overlay_bind to the
                    docker bridge address (typically 172.17.0.1) or leave it
                    on 0.0.0.0 and firewall the port from the internet.
  OVERLAY_TOKEN     token appended to every upstream request (empty = the
                    control server was configured with an empty token)
  OVERLAY_AUTH_USER / OVERLAY_AUTH_PASS
                    HTTP Basic auth required for every request (pages, API,
                    icons). Both empty = auth disabled. OBS browser sources
                    accept credential URLs: http://user:pass@host:8091/
  BIND / PORT       listen address, defaults 0.0.0.0 / 8091

Hardening: GET only, whitelisted API paths, only the instance_id query
parameter is forwarded (a client-supplied token is dropped), 5s upstream
timeout, and static file serving is restricted to the page files and the
ICONS/ directory with path-traversal protection.
"""

import base64
import hmac
import os
import urllib.error
import urllib.parse
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parent
UPSTREAM = os.environ.get("OVERLAY_UPSTREAM", "http://127.0.0.1:8090").rstrip("/")
TOKEN = os.environ.get("OVERLAY_TOKEN", "")
AUTH_USER = os.environ.get("OVERLAY_AUTH_USER", "")
AUTH_PASS = os.environ.get("OVERLAY_AUTH_PASS", "")

API_PATHS = {"/overlay", "/overlay/instances", "/overlay/skilltree", "/overlay/builds"}

PAGES = {
    "/": "spectator-overlay.html",
    "/spectator-overlay.html": "spectator-overlay.html",
    "/skilltree": "skill-tree-overlay.html",
    "/skill-tree-overlay.html": "skill-tree-overlay.html",
}

ICON_TYPES = {
    ".png": "image/png",
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".gif": "image/gif",
    ".webp": "image/webp",
    ".svg": "image/svg+xml",
}

MAX_UPSTREAM_BYTES = 2 * 1024 * 1024


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self) -> None:  # noqa: N802 (http.server API)
        if not self.check_auth():
            return
        parsed = urllib.parse.urlsplit(self.path)
        path = parsed.path

        if path in API_PATHS:
            self.proxy_api(path, parsed.query)
            return

        page = PAGES.get(path)
        if page:
            self.send_file(ROOT / page, "text/html; charset=utf-8")
            return

        if path.startswith("/ICONS/"):
            self.send_icon(path)
            return

        self.send_body(404, b'{"error":"not found"}', "application/json")

    def check_auth(self) -> bool:
        if not AUTH_USER and not AUTH_PASS:
            return True
        header = self.headers.get("Authorization", "")
        if header.startswith("Basic "):
            try:
                supplied = base64.b64decode(header[6:], validate=True)
            except (ValueError, UnicodeDecodeError):
                supplied = b""
            expected = f"{AUTH_USER}:{AUTH_PASS}".encode("utf-8")
            # Constant-time compare -- don't leak prefix length via timing.
            if hmac.compare_digest(supplied, expected):
                return True
        self.send_response(401)
        self.send_header("WWW-Authenticate",
                         'Basic realm="spectator-overlay", charset="UTF-8"')
        body = b'{"error":"authentication required"}'
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)
        return False

    def proxy_api(self, path: str, raw_query: str) -> None:
        params = urllib.parse.parse_qs(raw_query)
        forward: dict[str, str] = {}
        instance_id = params.get("instance_id", [""])[0]
        if instance_id:
            if not instance_id.lstrip("-").isdigit():
                self.send_body(400, b'{"error":"bad instance_id"}', "application/json")
                return
            forward["instance_id"] = instance_id
        if TOKEN:
            forward["token"] = TOKEN

        url = UPSTREAM + path
        if forward:
            url += "?" + urllib.parse.urlencode(forward)

        try:
            with urllib.request.urlopen(url, timeout=5) as response:
                body = response.read(MAX_UPSTREAM_BYTES)
            self.send_body(200, body, "application/json")
        except urllib.error.HTTPError as exc:
            # Pass upstream errors (403 bad token, 400 missing instance)
            # through so misconfiguration is visible in the page status line.
            self.send_body(exc.code, exc.read() or b"{}", "application/json")
        except OSError:
            self.send_body(502, b'{"error":"overlay upstream unreachable"}',
                           "application/json")

    def send_file(self, file_path: Path, content_type: str) -> None:
        try:
            body = file_path.read_bytes()
        except OSError:
            self.send_body(404, b'{"error":"not found"}', "application/json")
            return
        self.send_body(200, body, content_type)

    def send_icon(self, url_path: str) -> None:
        icons_root = (ROOT / "ICONS").resolve()
        relative = urllib.parse.unquote(url_path[len("/ICONS/"):])
        target = (icons_root / relative).resolve()
        # Path-traversal guard: the resolved target must stay under ICONS/.
        if icons_root not in target.parents and target != icons_root:
            self.send_body(404, b'{"error":"not found"}', "application/json")
            return
        content_type = ICON_TYPES.get(target.suffix.lower())
        if not content_type or not target.is_file():
            self.send_body(404, b'{"error":"not found"}', "application/json")
            return
        self.send_file(target, content_type)

    def send_body(self, code: int, body: bytes, content_type: str) -> None:
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt: str, *args) -> None:
        # Quiet the per-request lines for the 300ms poll loop; errors still
        # surface via send_error/exceptions on stderr.
        pass


def main() -> None:
    bind = os.environ.get("BIND", "0.0.0.0")
    port = int(os.environ.get("PORT", "8091"))
    server = ThreadingHTTPServer((bind, port), Handler)
    print(f"[overlay-proxy] listening on {bind}:{port}, upstream {UPSTREAM}, "
          f"token {'set' if TOKEN else 'EMPTY'}, "
          f"basic auth {'ON' if (AUTH_USER or AUTH_PASS) else 'OFF'}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
