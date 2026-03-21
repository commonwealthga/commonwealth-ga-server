---
phase: 07-control-server-protocol-migration
plan: 02
subsystem: control-server
tags: [tcp, protocol, migration, asio, linux]
dependency_graph:
  requires: [07-01]
  provides: [CTRL-02, CTRL-03]
  affects: []
tech_stack:
  added: []
  patterns:
    - ASIO steady_timer for async delay (replaces Win32 Sleep)
    - C++17 variadic template for append() (replaces C++20 auto parameter)
    - IPC stub comments marking Phase 10 integration points
key_files:
  created:
    - src/ControlServer/TcpSession/PacketView.hpp
    - src/ControlServer/TcpSession/TcpSession.hpp
    - src/ControlServer/TcpSession/TcpSession.cpp
    - src/ControlServer/TcpListener/TcpListener.hpp
  modified:
    - Makefile
decisions:
  - Sleep(1000) replaced with asio::steady_timer::async_wait -- non-blocking is required because the control server runs on a single io_context thread shared with TcpListener
  - Inventory::Get* methods (DLL-only) stubbed with empty DATA_SET responses and Phase 10 comment -- client proceeds without inventory data until IPC populates it
  - Config::GetIpChar/GetPort hardcoded to 127.0.0.1:9002 with "Phase 8 TODO" comment -- instance address registry is Phase 8 work
  - auto parameter template replaced with template<typename... Bytes> -- auto parameters require C++20; control server is built with -std=c++17
  - GTcpEvents/GBeaconPickupEvents/GBeaconRemoveEvents/GQuestEvents entirely removed -- DLL-specific in-process event maps have no equivalent in the control server; replaced with Phase 10 IPC stub comment in RELAY_LOG handler
metrics:
  duration: ~9 min
  completed: 2026-03-22
  tasks: 2
  files: 5
---

# Phase 7 Plan 02: TCP Protocol Migration Summary

Full migration of the GA TCP protocol from the DLL TcpSession into the native Linux control server, including PacketView binary reader, TcpSession (all 28 packet handlers), TcpListener accept loop, and Makefile update -- all compiling on g++ with no DLL dependencies.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Copy and adapt PacketView + TcpSession header | ec323e5 | PacketView.hpp (verbatim), TcpSession.hpp (adapted) |
| 2 | Migrate TcpSession.cpp + create TcpListener + update Makefile | 64c8fb5 | TcpSession.cpp, TcpListener.hpp, Makefile, TcpSession.hpp (fix) |

## Decisions Made

**ASIO steady_timer for deferred go_play:** The DLL used `Sleep(1000)` before sending go_play after character select. In the control server this would block the io_context thread. Replaced with `asio::steady_timer::async_wait` with a 1000ms delay, capturing `shared_from_this()` and the timer shared_ptr to keep both alive until the callback fires.

**Inventory stub with empty response:** `send_inventory_response` and `send_character_inventory_response` depend on `Inventory::GetEquippedByPawnId()` which is DLL-only (uses ATgPawn* pointers). For Phase 7 these return empty DATA_SET responses -- the client proceeds to the game without inventory data. Phase 10 IPC will populate these from game events.

**Config hardcoded to 127.0.0.1:9002:** `Config::GetIpChar()` and `Config::GetPort()` are DLL-only. The control server hardcodes the game server address for now with a Phase 8 TODO. Phase 8 will add an instance registry.

**auto parameter replaced with variadic template:** The `append(buffer, auto&&... bytes)` pattern is a C++20 abbreviated function template. The Makefile uses `-std=c++17`, which rejected it. Replaced with `template<typename... Bytes> void append(...)` which compiles cleanly.

**RELAY_LOG handler stubbed:** The DLL RELAY_LOG handler drained GTcpEvents, GBeaconPickupEvents, GBeaconRemoveEvents, and GQuestEvents -- all DLL-only in-process event maps with ATgPawn* keys. In the control server these have no equivalent. The handler now contains a single comment: "Phase 10: IPC GAME_EVENT will populate event queues here." RELAY_LOG functions as a keepalive-only packet until Phase 10.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] C++20 auto parameter used with C++17 build**
- **Found during:** Task 2 build
- **Issue:** `void append(std::vector<uint8_t>& buffer, auto&&... bytes)` requires C++20 or `-fconcepts`. The Makefile uses `-std=c++17`. This was in the original DLL TcpSession but the DLL compiler apparently accepted it as an extension.
- **Fix:** Replaced with `template<typename... Bytes> void append(...)` with explicit `static_cast<uint8_t>` in the body.
- **Files modified:** src/ControlServer/TcpSession/TcpSession.hpp
- **Commit:** 64c8fb5 (bundled with Task 2)

## Verification Results

```
grep DLL-specific includes: CLEAN (0 matches)
grep ATgPawn in TcpSession: CLEAN (0 matches, only comment in EquipSlot.hpp)
grep PlayerSessionStore in TcpSession.cpp: 6 usages
grep Sleep( in TcpSession.cpp: 0 (Should be 0) -- PASS
grep send_*_response methods: 28 methods
make control-server-linux: SUCCESS, no errors, no warnings
```

## Self-Check: PASSED

All created files exist on disk. All task commits verified in git log (ec323e5, 64c8fb5).
