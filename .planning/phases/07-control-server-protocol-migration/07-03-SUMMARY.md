---
phase: 07-control-server-protocol-migration
plan: 03
subsystem: infra
tags: [asio, ipc, tcp, control-server, sqlite, makefile]

# Dependency graph
requires:
  - phase: 07-01
    provides: Database, PlayerSessionStore, Logger, Constants, Makefile dual-target scaffolding
  - phase: 07-02
    provides: TcpSession (28 send_* methods), TcpListener, PacketView

provides:
  - IpcServer with 4-byte LE framing and PING/PONG dispatch (src/ControlServer/IpcServer/)
  - Fully wired main.cpp entry point with CLI args, Database::Init, PlayerSessionStore::Init, TcpListener, IpcServer
  - bInitTcpServer=false documented with Phase 7 comment in GameEngine__Init.cpp
  - Runnable out/control-server Linux ELF that starts, logs banner, and listens on ports 9000 and 9010

affects: [08-instance-manager, 10-ipc-game-events]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "IpcSession write_in_progress chain: one async_write in flight, deque for pending frames, same as IpcClient"
    - "shared_from_this throughout IpcSession async callbacks to keep session alive across read/write chains"
    - "Signal handler via global asio::io_context* pointer: SIGINT/SIGTERM calls io.stop()"
    - "CLI arg parsing: both --key=val and --key val forms handled with simple loop (no library)"

key-files:
  created:
    - src/ControlServer/IpcServer/IpcServer.hpp
    - src/ControlServer/IpcServer/IpcServer.cpp
  modified:
    - src/ControlServer/main.cpp
    - src/GameServer/Engine/GameEngine/Init/GameEngine__Init.cpp
    - Makefile

key-decisions:
  - "IpcSession defined in IpcServer.cpp (not in .hpp) -- private to translation unit, no external linkage needed"
  - "write_in_progress chain instead of strand -- IpcSession is single-threaded (all ops on ASIO thread), no contention"
  - "Phase 10 stub comment in dispatch() rather than empty else -- explicit marker for future GAME_EVENT/PLAYER_SPAWNED work"

patterns-established:
  - "ASIO session pattern: enable_shared_from_this, start() -> do_read_header -> do_read_body -> dispatch -> do_read_header loop"
  - "Control server CLI: --key=val and --key val both handled; --help exits with usage"

requirements-completed: [CTRL-01, CTRL-02, CTRL-03, CTRL-04]

# Metrics
duration: 12min
completed: 2026-03-22
---

# Phase 7 Plan 03: IpcServer, main.cpp wiring, and DLL TCP cutover Summary

**Standalone control server binary that listens on port 9000 (GA clients) and port 9010 (game instances via IpcServer), with ASIO PING/PONG IPC, CLI arg parsing, and full make all build verification**

## Performance

- **Duration:** ~12 min
- **Started:** 2026-03-22T01:00:00Z
- **Completed:** 2026-03-22T01:12:00Z
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments

- IpcServer accepts game instance connections on port 9010; IpcSession reads 4-byte LE frames and dispatches PING -> PONG using shared IpcProtocol.hpp and IpcFraming.hpp
- main.cpp fully wired: Database::SetDbPath, Database::Init, PlayerSessionStore::Init, TcpListener, IpcServer, io.run() with SIGINT/SIGTERM clean shutdown and CLI args (--port, --ipc-port, --db-path, --help)
- Full `make all` build verified clean: out/version.dll (DLL), out/client/version.dll (client DLL), out/control-server (ELF); runtime test confirms startup banner and port binding

## Task Commits

1. **Task 1: IpcServer implementation** - `170c04c` (feat)
2. **Task 2: main.cpp wiring with CLI args** - `667c94c` (feat)
3. **Task 3: DLL TCP cutover + full build verification** - `1c2b7fb` (feat)

## Files Created/Modified

- `src/ControlServer/IpcServer/IpcServer.hpp` - IpcServer class declaration (ASIO acceptor on configurable port)
- `src/ControlServer/IpcServer/IpcServer.cpp` - IpcSession (4-byte LE framing, PING/PONG, write_in_progress chain) + IpcServer::do_accept()
- `src/ControlServer/main.cpp` - Full entry point: CLI parsing, Database::Init, PlayerSessionStore::Init, TcpListener, IpcServer, SIGINT/SIGTERM, io.run()
- `src/GameServer/Engine/GameEngine/Init/GameEngine__Init.cpp` - Added Phase 7 comment above bInitTcpServer=false
- `Makefile` - Added src/ControlServer/IpcServer/IpcServer.cpp to CS_SOURCES

## Decisions Made

- IpcSession defined in IpcServer.cpp only -- no external symbol needed; keeps header minimal
- write_in_progress chain used for IpcSession send (same pattern as IpcClient) -- prevents concurrent async_write on same socket
- Phase 10 stub comment in dispatch() marks where GAME_EVENT/PLAYER_SPAWNED handlers will be added

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. All three tasks built and verified clean on the first attempt.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 7 complete: control server is a running binary handling all client TCP connections
- Phase 8 (instance manager) can reference IpcServer as the counterpart to IpcClient in the DLL
- IpcServer dispatch() has Phase 10 stub comment where GAME_EVENT/PLAYER_SPAWNED handlers will be added
- No blockers for Phase 8

---
*Phase: 07-control-server-protocol-migration*
*Completed: 2026-03-22*

## Self-Check: PASSED

- FOUND: src/ControlServer/IpcServer/IpcServer.hpp
- FOUND: src/ControlServer/IpcServer/IpcServer.cpp
- FOUND: src/ControlServer/main.cpp
- FOUND: commit 170c04c (IpcServer)
- FOUND: commit 667c94c (main.cpp)
- FOUND: commit 1c2b7fb (DLL cutover)
