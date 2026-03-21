---
phase: 07-control-server-protocol-migration
plan: 01
subsystem: control-server
tags: [build-system, logging, database, sqlite, session-management]
dependency_graph:
  requires: []
  provides:
    - control-server-linux target (out/control-server ELF)
    - control-server-win target (out/control-server.exe PE)
    - src/ControlServer/Logger.hpp
    - src/ControlServer/Database/Database.hpp
    - src/ControlServer/Database/Database.cpp
    - src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp
    - src/ControlServer/PlayerSessionStore/PlayerSessionStore.cpp
    - src/ControlServer/Constants/ (5 files)
  affects: []
tech_stack:
  added:
    - native x86_64 g++ target for control server
    - x86_64-w64-mingw32-g++ cross-compile target
    - sqlite3.c compiled separately with gcc (not g++) to avoid C++ void* strictness
  patterns:
    - Static class singletons with Init() (same as DLL pattern)
    - std::mutex + std::lock_guard instead of CRITICAL_SECTION
    - inline static member definitions in header-only Logger
key_files:
  created:
    - Makefile (appended control-server targets)
    - src/ControlServer/Logger.hpp
    - src/ControlServer/main.cpp
    - src/ControlServer/Constants/TcpFunctions.h
    - src/ControlServer/Constants/TcpTypes.h
    - src/ControlServer/Constants/GameTypes.h
    - src/ControlServer/Constants/EquipSlot.hpp
    - src/ControlServer/Constants/DeviceIds.hpp
    - src/ControlServer/Constants/DeviceIds.cpp
    - src/ControlServer/Database/Database.hpp
    - src/ControlServer/Database/Database.cpp
    - src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp
    - src/ControlServer/PlayerSessionStore/PlayerSessionStore.cpp
  modified:
    - Makefile
decisions:
  - sqlite3.c compiled with gcc not g++ -- g++ void* strictness generates hundreds of errors in sqlite3.c amalgamation; separate object file avoids the issue
  - CS_CPP_SOURCES separate from sqlite C source -- allows clean $(filter %.cpp) vs .o pattern
  - inline static member definitions in Logger.hpp -- avoids ODR violations when header included across multiple TUs; log-all-by-default when EnabledChannels is empty
  - Database.cpp init check for null db before Init() -- prevents crash if sqlite3_open fails
  - SessionInfo drops player_name_w (wstring) -- control server has no UE3 SDK or Win32 wide-char dependency
metrics:
  duration_seconds: 512
  tasks_completed: 3
  tasks_total: 3
  files_created: 13
  files_modified: 1
  completed_date: "2026-03-22"
---

# Phase 7 Plan 01: Control Server Skeleton Summary

**One-liner:** Dual-target (Linux ELF + Windows PE) control server build with portable Logger, SQLite WAL database, and mutex-protected PlayerSessionStore -- all without pch.hpp or DLL SDK headers.

## Objective

Establish the compilable foundation for the standalone control server binary. Everything Plan 02 (TcpSession migration) and Plan 03 (main.cpp wiring) build upon is created here.

## What Was Built

### Makefile Targets
- `control-server-linux`: native g++ producing `out/control-server` ELF
- `control-server-win`: x86_64-w64-mingw32-g++ producing `out/control-server.exe` PE
- `control-server`: builds both
- `cleancs`: removes both binaries and intermediate sqlite3 objects
- sqlite3.c compiled separately with gcc (`out/sqlite3_cs_linux.o`, `out/sqlite3_cs_win.o`) to avoid C++ void* conversion errors

### Logger.hpp
Portable header-only Logger writing to stderr. Same API as the DLL Logger: `Logger::Log(channel, fmt, ...)` and `Logger::GetTime()`. Uses `inline` static members for ODR safety. When `EnabledChannels` is empty, all channels are logged.

### Constants Files (src/ControlServer/Constants/)
- `TcpFunctions.h`, `TcpTypes.h`: pure enum namespaces, copied verbatim
- `EquipSlot.hpp`: copied verbatim (no DLL deps)
- `GameTypes.h`: copied, DeviceIds include path changed to ControlServer
- `DeviceIds.hpp`: copied verbatim
- `DeviceIds.cpp`: new file with ControlServer includes for Database and Logger

### Database.hpp / Database.cpp
Full migration from `src/Database/` with:
- `SetDbPath(path)` for CLI configuration
- WAL + busy_timeout=5000 + synchronous=NORMAL pragmas applied on every `GetConnection()` open
- All 14 schema migration versions preserved for schema compatibility with shared server.db
- `PlayerRegistry::Init()` call removed -- PlayerSessionStore::Init() called separately from main.cpp
- No pch.hpp, no PlayerRegistry, no DLL SDK headers

### PlayerSessionStore.hpp / PlayerSessionStore.cpp
Migration from `src/GameServer/Storage/PlayerRegistry/` with:
- `SessionInfo` struct (mirrors PlayerInfo without wstring)
- `CharacterInfo` struct identical to DLL version
- `std::mutex` + `std::lock_guard<std::mutex>` instead of CRITICAL_SECTION + InitializeCriticalSection
- All DB queries through `Database::GetConnection()`
- Full API: Init, Register, Unregister, GetByGuid, GetByIp, GetByGuidPtr, UpsertUser, InsertCharacter, GetCharactersByUserId, GetCharacterById, SetSelectedCharacter

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] sqlite3.c compiled with g++ instead of gcc**
- **Found during:** Task 1 first build attempt
- **Issue:** `CS_SOURCES` included `lib/sqlite3/sqlite3.c` passed directly to g++, which rejects hundreds of `void*` -> typed-pointer conversions in the sqlite3 amalgamation
- **Fix:** Separated sqlite3 into its own Makefile rules compiled with `gcc`/`x86_64-w64-mingw32-gcc`, producing `out/sqlite3_cs_linux.o` and `out/sqlite3_cs_win.o` object files linked separately
- **Files modified:** Makefile
- **Commit:** 7adfa00

## Self-Check: PASSED

Files verified present:
- src/ControlServer/Logger.hpp: FOUND
- src/ControlServer/Database/Database.hpp: FOUND
- src/ControlServer/Database/Database.cpp: FOUND
- src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp: FOUND
- src/ControlServer/PlayerSessionStore/PlayerSessionStore.cpp: FOUND

Commits verified:
- 7adfa00: FOUND (Task 1 - Makefile + Logger + constants stubs)
- dfd0864: FOUND (Task 2 - Database full implementation)
- 985da36: FOUND (Task 3 - PlayerSessionStore full implementation)

Build verified:
- make control-server-linux: PASSED (out/control-server)
- make control-server-win: PASSED (out/control-server.exe)
