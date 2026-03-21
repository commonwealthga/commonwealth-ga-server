---
phase: 06-thread-safety-ipc-plumbing
plan: 02
subsystem: ipc
tags: [ipc, asio, tcp-client, reconnect, queue, actor-tick, config]

# Dependency graph
requires:
  - 06-01 (IpcProtocol.hpp, IpcFraming.hpp, nlohmann/json)
provides:
  - IpcClient static class: Init(), Send(), DrainInbound(), IpcThread()
  - ASIO background thread with exponential backoff reconnect (2s->4s->...->30s cap)
  - Non-blocking outbound queue (CRITICAL_SECTION, drained on ASIO thread)
  - Inbound swap-drain pattern (game thread calls DrainInbound each tick)
  - Config::GetIpcPort() default 9010, --ipcport CLI arg
  - Config::GetIpcHost() default 127.0.0.1, --ipchost CLI arg
  - Actor__Tick hook active, drains IPC inbound queue each UE3 tick
affects: [07-control-server, 08-player-register-ipc, 09-instance-manager]

# Tech tracking
tech-stack:
  added: [IpcClient ASIO TCP client, write_in_progress chain pattern]
  patterns:
    - CRITICAL_SECTION + IpcCsGuard RAII instead of std::mutex (MinGW win32 threading model)
    - write_in_progress chain: only one async_write in flight at a time
    - asio::post() from game thread to ASIO thread for safe write kicks
    - swap-drain pattern in DrainInbound (minimal lock hold time)

key-files:
  created:
    - src/IpcClient/IpcClient.hpp
    - src/IpcClient/IpcClient.cpp
  modified:
    - src/GameServer/Engine/Actor/Tick/Actor__Tick.cpp
    - src/Config/Config.hpp
    - src/Config/Config.cpp
    - src/dllmain.cpp
    - Makefile

key-decisions:
  - "CRITICAL_SECTION used instead of std::mutex -- MinGW i686-w64-mingw32-g++ uses win32 threading model, not posix"
  - "write_in_progress chain: async_write never called while another is in flight (ASIO-thread-only flag)"
  - "asio::post() used from Send() to kick ASIO thread safely -- never touch socket from game thread"
  - "io_ctx_ pointer read from game thread is a benign race: worst case is a missed kick, queued message sent on next reconnect"
  - "Actor__Tick hook activated -- game thread now drains IPC inbound queue each tick via DrainInbound()"
  - "Default IPC port 9010 avoids 9000 (TcpServer), 9001 (ChatServer), 9002 (UDP)"

requirements-completed: [IPC-01, IPC-03]

# Metrics
duration: 7min
completed: 2026-03-21
---

# Phase 6 Plan 2: IpcClient ASIO Thread, Queues, and Reconnect Summary

**Full IPC transport layer: ASIO background thread, non-blocking queue-based send/recv, auto-reconnect with exponential backoff, game-thread drain via Actor__Tick**

## Performance

- **Duration:** 7 min
- **Completed:** 2026-03-21
- **Tasks:** 2
- **Files modified:** 7 (2 created, 5 modified)

## Accomplishments

- Implemented `IpcClient` as a static class with an ASIO background thread that connects to the control server over loopback TCP on port 9010
- Outbound queue uses CRITICAL_SECTION (not std::mutex -- MinGW win32 threading model) with the write_in_progress chain pattern ensuring only one `async_write` is in flight at a time
- Inbound messages are pushed by the ASIO thread and swap-drained by the game thread via `IpcClient::DrainInbound()` called from Actor__Tick each tick
- Exponential backoff reconnect loop: 2s, 4s, 8s, ... capped at 30s
- Sends initial PING on connect; DrainInbound handles PONG and logs it
- Config methods `GetIpcPort()` and `GetIpcHost()` added with --ipcport/--ipchost CLI args and sensible defaults (9010, 127.0.0.1)
- Actor__Tick hook uncommented and activated in dllmain
- `IpcClient::Init()` called in ModuleThread after `::DetourTransactionCommit()`
- Build verified: `make cleanserver && make all` succeeds with zero errors

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Critical Constraint] CRITICAL_SECTION substituted for std::mutex**

- **Found during:** Task 1 implementation (pre-empted by CRITICAL_CONSTRAINT in prompt)
- **Issue:** Plan specified `#include <mutex>`, `std::mutex`, `std::lock_guard<std::mutex>` but MinGW i686-w64-mingw32-g++ uses the win32 threading model which does not provide `<mutex>`
- **Fix:** Used `CRITICAL_SECTION` + custom `IpcCsGuard` RAII struct (matching the pattern from PlayerRegistry.cpp established in 06-01)
- **Files modified:** src/IpcClient/IpcClient.hpp, src/IpcClient/IpcClient.cpp
- **Commit:** 917a8cc

## Self-Check: PASSED

- src/IpcClient/IpcClient.hpp: FOUND
- src/IpcClient/IpcClient.cpp: FOUND
- Task 1 commit 917a8cc: FOUND
- Task 2 commit 88fa2ad: FOUND
- `make cleanserver && make all`: zero errors, both version.dll files linked
