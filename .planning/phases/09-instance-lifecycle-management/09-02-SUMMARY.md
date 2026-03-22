---
phase: 09-instance-lifecycle-management
plan: "02"
subsystem: control-server
tags: [instance-spawner, ipc-routing, multi-session, fork-exec, wine]
dependency_graph:
  requires:
    - "09-01: ControlServerConfig, InstanceRegistry, IpcProtocol constants"
  provides:
    - "InstanceSpawner::Spawn (fork/exec with Wine)"
    - "IpcServer multi-session routing keyed by instance_id"
    - "INSTANCE_HELLO/INSTANCE_READY handshake"
    - "main.cpp config-driven startup with home map spawn"
  affects:
    - "src/ControlServer/TcpSession/TcpSession.cpp (SendToInstance call site updated)"
tech_stack:
  added:
    - "fork/exec (POSIX) for Wine child process spawning"
    - "SIGCHLD SIG_IGN for automatic zombie reaping"
  patterns:
    - "g_sessions map: std::map<int64_t, std::weak_ptr<IpcSession>> for per-instance routing"
    - "IpcSession::on_disconnect: cleans g_sessions and calls MarkStopped"
    - "static members on IpcSession (same TU as g_sessions, avoids header pollution)"
key_files:
  created:
    - path: "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
      note: "Rewritten stub with correct pid_t Spawn(cfg, map, mode, port, instance_id) signature"
    - path: "src/ControlServer/InstanceSpawner/InstanceSpawner.cpp"
      note: "fork/exec with Wine env vars, xvfb-run, -instanceid/-ipcport CLI args"
  modified:
    - path: "src/ControlServer/IpcServer/IpcServer.hpp"
      note: "SendToInstance signature changed to (int64_t instance_id, const string& json)"
    - path: "src/ControlServer/IpcServer/IpcServer.cpp"
      note: "g_active_ipc_session replaced with g_sessions map; INSTANCE_HELLO/READY dispatch; disconnect cleanup"
    - path: "src/ControlServer/main.cpp"
      note: "Config load from JSON, SIGCHLD, ClearStaleInstances, AllocatePort, InsertStarting, Spawn"
    - path: "src/ControlServer/TcpSession/TcpSession.cpp"
      note: "SendToInstance call updated to use GetReadyHomeInstance() for instance_id lookup"
decisions:
  - "g_sessions/sessions_mutex_ declared as IpcSession static members (same TU) -- avoids IpcServer.hpp pollution, IpcServer accesses via friend"
  - "on_disconnect() factored out of do_read_header/do_read_body error paths -- single call handles g_sessions cleanup and MarkStopped"
  - "InstanceSpawner stub signature from Plan 01 replaced entirely -- pid_t return instead of int64_t; udp_port/instance_id passed explicitly"
  - "main.cpp config-only: --port/--ipc-port/--game-port/--db-path flags removed, all values come from JSON config"
metrics:
  duration: "~8 min"
  completed: "2026-03-22"
  tasks_completed: 2
  files_modified: 6
---

# Phase 9 Plan 02: InstanceSpawner + IpcServer Multi-Session Summary

**One-liner:** Fork/exec Wine child process per UE3 instance with per-instance IPC session routing via INSTANCE_HELLO/READY handshake.

## What Was Built

### Task 1: InstanceSpawner + IpcServer multi-session

**InstanceSpawner::Spawn** -- replaces the stub from Plan 01 with a real fork/exec:
- Sets `WINEDLLOVERRIDES` and `WINEPREFIX` from config
- Builds argv: `xvfb-run -a wine game.exe server MapName?Game=Mode -nodatabase -unattended -host=... -port=... -seekfreeloading -tcp=300 -nullrhi -ipcport=... -instanceid=...`
- Named local `std::string` variables prevent dangling `c_str()` in argv
- Child calls `_exit(1)` on execvp failure (not `exit()`) to avoid flushing parent stdio buffers
- Parent logs and returns pid

**IpcServer refactor** -- multi-session routing:
- `g_active_ipc_session` (single weak_ptr) replaced with `IpcSession::g_sessions` (`std::map<int64_t, std::weak_ptr<IpcSession>>`)
- `INSTANCE_HELLO` handler: validates `instance_id != 0`, sets `instance_id_`/`validated_`, inserts into `g_sessions`, sends `INSTANCE_HELLO_ACK`
- `INSTANCE_READY` handler: calls `InstanceRegistry::MarkReady(inst_id, max_players)`
- `on_disconnect()`: removes from `g_sessions`, calls `InstanceRegistry::MarkStopped`
- `SendToInstance(int64_t instance_id, json)`: looks up in g_sessions, handles expired weak_ptr
- `do_accept`: no longer stores session globally -- sessions are anonymous until INSTANCE_HELLO

### Task 2: main.cpp wiring

- `--config=PATH` replaces all previous CLI flags; `ControlServerConfig::Load(path)` provides all values
- `SIGCHLD SIG_IGN` added after SIGINT/SIGTERM for automatic zombie reaping
- Startup sequence: `ClearStaleInstances` → `AllocatePort` → `InsertStarting` → `InstanceSpawner::Spawn`
- Does NOT wait for INSTANCE_READY synchronously -- io.run() processes it asynchronously when the DLL sends it

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] TcpSession::initiate_player_register_and_go_play SendToInstance call**
- **Found during:** Task 1 (changed SendToInstance signature from string to instance_id+string)
- **Issue:** TcpSession.cpp called `IpcServer::SendToInstance(reg.dump())` with old single-arg signature; would not compile with new signature
- **Fix:** Added `GetReadyHomeInstance()` lookup before the call; routes PLAYER_REGISTER to the home map instance's session by instance_id. Added early-return if no READY home instance exists.
- **Files modified:** `src/ControlServer/TcpSession/TcpSession.cpp`
- **Commit:** fb8d174

**2. [Rule 1 - Bug] InstanceSpawner.hpp stub signature mismatch**
- **Found during:** Task 1
- **Issue:** Plan 01 stub used `int64_t Spawn(cfg, map, mode, is_home_map)` but Plan 02 spec requires `pid_t Spawn(cfg, map, mode, udp_port, instance_id)`; the stub was incompatible with caller in main.cpp
- **Fix:** Rewrote the header with the correct signature; this was part of the planned task scope
- **Commit:** fb8d174

## Self-Check: PASSED

| Check | Result |
|-------|--------|
| InstanceSpawner.hpp | FOUND |
| InstanceSpawner.cpp | FOUND |
| IpcServer.hpp | FOUND |
| IpcServer.cpp | FOUND |
| main.cpp | FOUND |
| Commit fb8d174 | FOUND |
| Commit 67323e8 | FOUND |
| control-server-linux build | PASSED (no errors, no warnings) |
