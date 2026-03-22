---
phase: 08-player-registration-udp-redirect
plan: "01"
subsystem: control-server
tags: [ipc, instance-registry, sqlite, shared-headers]
dependency_graph:
  requires: []
  provides: [IpcProtocol.MSG_PLAYER_REGISTER, HexUtils.hex_encode, InstanceRegistry.GetReadyInstance, InstanceRegistry.SeedHomeMapInstance]
  affects: [src/Shared/IpcProtocol.hpp, src/Shared/HexUtils.hpp, src/ControlServer/InstanceRegistry, src/ControlServer/Database/Database.cpp, src/ControlServer/main.cpp, Makefile]
tech_stack:
  added: [HexUtils.hpp (header-only hex encode/decode), InstanceRegistry (SQLite-backed static class)]
  patterns: [PlayerSessionStore mutex pattern, sqlite3_prepare_v2/step/finalize, DB migration chain]
key_files:
  created:
    - src/Shared/HexUtils.hpp
    - src/ControlServer/InstanceRegistry/InstanceRegistry.hpp
    - src/ControlServer/InstanceRegistry/InstanceRegistry.cpp
  modified:
    - src/Shared/IpcProtocol.hpp
    - src/ControlServer/Database/Database.cpp
    - src/ControlServer/main.cpp
    - Makefile
decisions:
  - InstanceRegistry::Init() is a no-op (mutex default-constructed) for symmetry with PlayerSessionStore pattern
  - SeedHomeMapInstance marks all non-STOPPED rows as STOPPED before inserting fresh READY row, handling crash-recovery cleanly
  - ga_instances.ip_address defaults to 127.0.0.1 for single-machine deployment; multi-host is a later concern
  - --game-port default is 9002 (matches the UDP port Config.cpp hardcodes in Phase 07-02)
metrics:
  duration: "~4 min"
  completed_date: "2026-03-22"
  tasks_completed: 2
  files_changed: 7
---

# Phase 8 Plan 1: Instance Registry Foundation Summary

SQLite-backed InstanceRegistry with ga_instances table (DB v15), cross-platform HexUtils, PLAYER_REGISTER IPC constants, and --game-port startup wiring for the home map instance seed.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Extend IpcProtocol.hpp and create HexUtils.hpp | 2656fdc | src/Shared/IpcProtocol.hpp, src/Shared/HexUtils.hpp |
| 2 | InstanceRegistry + DB migration v15 + main.cpp wiring + Makefile | 6bef411 | src/ControlServer/InstanceRegistry/*, Database.cpp, main.cpp, Makefile |

## What Was Built

### IpcProtocol.hpp (extended)
Added `MSG_PLAYER_REGISTER = "PLAYER_REGISTER"` and `MSG_PLAYER_REGISTER_ACK = "PLAYER_REGISTER_ACK"` constants. The "Future" comment was replaced with the real constants; remaining futures (INSTANCE_READY, GAME_EVENT) remain as comments.

### HexUtils.hpp (new, header-only)
`hex_encode(uint8_t*, size_t)`, `hex_encode(vector<uint8_t>)`, `hex_decode(string)` in `namespace HexUtils`. Compiles on both i686 (DLL, Windows/Wine) and x86_64 (control server, Linux) using only standard C++ headers.

### InstanceRegistry (new class)
- `InstanceInfo` struct: id, map_name, state, pid, udp_port, ip_address, player_count, started_at, sealed_at
- `GetReadyInstance(map_name)`: queries `ga_instances WHERE state='READY' LIMIT 1`, returns `std::optional<InstanceInfo>`
- `SeedHomeMapInstance(map_name, udp_port)`: stops stale rows, inserts fresh READY row, logs the result

### Database.cpp (migration v15)
New migration block creates `ga_instances` table with all lifecycle columns and an index on `state`. Final version bumped from 14 to 15.

### main.cpp wiring
`--game-port=N` / `--game-port N` CLI arg added (default 9002). After `PlayerSessionStore::Init()`: calls `InstanceRegistry::Init()` then `InstanceRegistry::SeedHomeMapInstance("Rot_Redistribution05", game_port)`. Startup log includes game port.

### Makefile
`$(CS_SRC_DIR)/InstanceRegistry/InstanceRegistry.cpp` added to `CS_CPP_SOURCES`.

## Verification Results

1. `make control-server-linux` -- PASS (no warnings, no errors)
2. `IpcProtocol.hpp` compiles with `i686-w64-mingw32-g++` and `g++` -- PASS
3. `HexUtils.hpp` compiles with `i686-w64-mingw32-g++` and `g++` -- PASS
4. `./out/control-server --help` shows `--game-port=N` option -- PASS
5. `./out/control-server --game-port=7777` against existing server.db logs `Schema at version 15` and `Seeded instance: map=Rot_Redistribution05 udp_port=7777` -- PASS

## Deviations from Plan

None - plan executed exactly as written.

## Self-Check

### Files created/exist:
- src/Shared/HexUtils.hpp: FOUND
- src/ControlServer/InstanceRegistry/InstanceRegistry.hpp: FOUND
- src/ControlServer/InstanceRegistry/InstanceRegistry.cpp: FOUND

### Commits exist:
- 2656fdc: FOUND
- 6bef411: FOUND

## Self-Check: PASSED
