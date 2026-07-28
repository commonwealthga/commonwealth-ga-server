# Spectator broadcast overlay

Browser-source overlay pages for streaming matches, fed by the control
server's overlay HTTP endpoint (see `OverlayHttpServer`). Data only flows for
instances that currently have a connected in-game spectator (`-spectate`).

## Pieces

- `spectator-overlay.html` — health bars, effects, class icons, build notation
- `skill-tree-overlay.html` — per-player skill-tree grids
- `overlay_proxy.py` — thin public-facing proxy: serves the two pages and
  forwards their `/overlay*` API calls to the control server, attaching the
  secret token **server-side** from the `OVERLAY_TOKEN` env var. The token
  never appears in the pages or in any browser-visible URL.

## Public hosting (docker)

1. In `control-server.json` set `overlay_http_port` (e.g. `8090`) and a real
   `overlay_token` (`openssl rand -hex 32`). Keep that port unreachable from
   the internet (firewall, or `overlay_bind` to a private address the
   container can reach — note `127.0.0.1` is *not* reachable from a
   container via `host.docker.internal`).
2. Create `.env` in this directory:

   ```
   OVERLAY_TOKEN=<the same token>
   OVERLAY_AUTH_USER=<pick a username>
   OVERLAY_AUTH_PASS=<pick a password>
   ```

   The user/pass enable HTTP Basic auth on everything the proxy serves;
   leave both out for a fully public overlay. In OBS use
   `http://user:pass@host:8091/` as the browser-source URL.
3. Optionally drop an icon pack in `./ICONS/` (`CLASSES/`, `EFFECTS/`,
   `SKILLS/` subfolders with `<id>.png`).
4. `docker compose up -d --build` → pages at `http://<host>:8091/` (health
   bars) and `http://<host>:8091/skilltree`. Put those URLs (or a
   reverse-proxied HTTPS version of them) straight into an OBS browser
   source.

Only `8091` should be public. The overlay JSON contains no credentials
(players are identified by an opaque `player_key` hash, not their session
GUID), and with `OVERLAY_AUTH_USER`/`OVERLAY_AUTH_PASS` set, nothing —
pages, API, instance list, icons — is served without credentials. Basic auth
sends credentials with every request, so put a TLS reverse proxy in front if
the overlay crosses untrusted networks.

## Direct / LAN use (no docker)

Set `OVERLAY_BASE_URL` in the page(s) to
`http://<control-host>:<overlay_http_port>/overlay` and `OVERLAY_TOKEN` to
the token from `control-server.json`, then open the file locally or as an
OBS browser source. Only do this on a network where exposing the token in
the page/URL is acceptable.
