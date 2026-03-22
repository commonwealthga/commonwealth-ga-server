---
phase: 09-instance-lifecycle-management
plan: "01"
subsystem: control-server
tags: [config, ipc-protocol, instance-registry, database-migration, sqlite]
dependency_graph:
  requires: []
  provides:
    - ControlServerConfig struct with JSON loader and per-field defaults
    - MSG_INSTANCE_HELLO, MSG_INSTANCE_HELLO_ACK, MSG_INSTANCE_READY IPC constants
    - InstanceRegistry InsertStarting/MarkReady/GetReadyHomeInstance/MarkStopped/AllocatePort/ClearStaleInstances
    - Database schema v16 with instance_id, is_home_map, max_players columns
    - InstanceSpawner stub (Plan 02 fills in)
  affects:
    - src/ControlServer/main.cpp (will call ClearStaleInstances + InsertStarting in Plan 02)
    - src/ControlServer/IpcServer/IpcServer.cpp (will use INSTANCE_HELLO constants in Plan 03)
tech_stack:
  added: [nlohmann/json for config parsing]
  patterns:
    - JSON config file with per-field defaults and graceful fallback on missing/invalid file
    - rowid-as-instance-id pattern (INSERT then UPDATE instance_id = last_insert_rowid)
    - COALESCE for new columns in existing SELECT queries (migration-safe reads)
key_files:
  created:
    - src/ControlServer/Config/ControlServerConfig.hpp
    - src/ControlServer/Config/ControlServerConfig.cpp
    - src/ControlServer/InstanceSpawner/InstanceSpawner.hpp
    - src/ControlServer/InstanceSpawner/InstanceSpawner.cpp
  modified:
    - src/Shared/IpcProtocol.hpp
    - src/ControlServer/InstanceRegistry/InstanceRegistry.hpp
    - src/ControlServer/InstanceRegistry/InstanceRegistry.cpp
    - src/ControlServer/Database/Database.cpp
    - Makefile
decisions:
  - rowid-as-instance-id: INSERT then UPDATE instance_id=rowid avoids needing a sequence or UUID; rowid is already unique and stable within the DB lifetime
  - AllocatePort holds mutex for entire read-then-decide: no TOCTOU window between checking in-use ports and returning a port
  - ClearStaleInstances at startup: crash-recovery pattern that ensures no STARTING/READY rows survive a restart without going through INSTANCE_READY
  - InstanceSpawner stub added to Makefile now so Plan 02 only needs to create the file
metrics:
  duration: "~3 min"
  completed_date: "2026-03-22"
  tasks_completed: 2
  files_changed: 9
---

# Phase 9 Plan 01: Foundational Contracts for Instance Lifecycle Summary

**One-liner:** JSON config loader, INSTANCE_HELLO/READY IPC constants, and InstanceRegistry with port pool and state-machine methods backed by Database migration v16.

## What Was Built

### Task 1 (committed 7f94941 -- prior session)
- `ControlServerConfig.hpp`: `PortRange` + `ControlServerConfig` struct with all defaults (wine binary, game binary, host, port range, timeouts, db path)
- `ControlServerConfig.cpp`: `Load()` using `nlohmann::json::parse` with graceful fallback; per-field `contains()` checks
- `IpcProtocol.hpp`: Added `MSG_INSTANCE_HELLO`, `MSG_INSTANCE_HELLO_ACK`, `MSG_INSTANCE_READY`; removed stale Future comment; kept `MSG_GAME_EVENT`/`MSG_PLAYER_SPAWNED` note
- `Makefile`: Added `Config/ControlServerConfig.cpp` and `InstanceSpawner/InstanceSpawner.cpp` to CS_CPP_SOURCES

### Task 2 (committed 7b4b3ca)
- `InstanceRegistry.hpp`: Added `instance_id`, `is_home_map`, `max_players` to `InstanceInfo`; declared `InsertStarting`, `MarkReady`, `GetReadyHomeInstance`, `MarkStopped`, `AllocatePort`, `ClearStaleInstances`
- `InstanceRegistry.cpp`: Implemented all six new methods; updated `GetReadyInstance` to fetch new columns via COALESCE
- `Database.cpp`: Migration v16 -- `ALTER TABLE ga_instances ADD COLUMN` for `instance_id`, `is_home_map`, `max_players`; partial-failure tolerant (each ALTER continues even if one fails); unique index on `instance_id WHERE instance_id != 0`; version bumped to 16
- `InstanceSpawner.hpp/.cpp`: Minimal stub returning 0 with log message; full implementation deferred to Plan 02

## Deviations from Plan

None -- plan executed exactly as written. Task 1 was already committed from a prior partial session (`7f94941`); Task 2 was the full remaining implementation.

## Self-Check: PASSED

All files exist on disk. Both commits (7f94941 and 7b4b3ca) present in git log. Control server builds cleanly.
