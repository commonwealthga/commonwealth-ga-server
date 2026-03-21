---
phase: 06-thread-safety-ipc-plumbing
plan: 01
subsystem: ipc
tags: [nlohmann-json, mutex, ipc, framing, protocol, thread-safety]

# Dependency graph
requires: []
provides:
  - IpcProtocol.hpp with MSG_PING, MSG_PONG, DEFAULT_IPC_PORT=9010 (arch-independent)
  - IpcFraming.hpp with 4-byte LE length-prefix Encode/Decode (arch-independent)
  - nlohmann/json 3.12.0 vendored at lib/nlohmann/json.hpp
  - PlayerRegistry mutex-protected on all 11 public methods
affects: [06-02-ipc-client, 07-control-server, 08-player-register-ipc]

# Tech tracking
tech-stack:
  added: [nlohmann/json 3.12.0, std::mutex (first mutex in codebase)]
  patterns: [src/Shared/ for arch-independent cross-process headers, lock_guard at top of each public method]

key-files:
  created:
    - src/Shared/IpcProtocol.hpp
    - src/Shared/IpcFraming.hpp
    - lib/nlohmann/json.hpp
  modified:
    - src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp
    - src/GameServer/Storage/PlayerRegistry/PlayerRegistry.cpp

key-decisions:
  - "Single std::mutex for all PlayerRegistry maps (by_guid_ and by_ip_ always updated together)"
  - "src/Shared/ headers use only standard C++ (no windows.h, no pch.hpp) to compile on both i686 DLL and x86_64 control server"
  - "4-byte LE length prefix matches existing TcpSession framing convention"

patterns-established:
  - "src/Shared/ pattern: headers that compile on both DLL (i686/Wine) and control server (x86_64/Linux) use only <cstdint>, <string>, <vector>, <cstring>"
  - "lock_guard at top of each public method body -- protects both in-memory maps and SQLite calls"

requirements-completed: [IPC-02, IPC-04]

# Metrics
duration: 2min
completed: 2026-03-21
---

# Phase 6 Plan 1: Thread Safety + IPC Plumbing Foundation Summary

**nlohmann/json vendored, IpcProtocol/IpcFraming shared headers created, PlayerRegistry mutex-hardened on all 11 public methods**

## Performance

- **Duration:** 2 min
- **Started:** 2026-03-21T19:32:47Z
- **Completed:** 2026-03-21T19:34:57Z
- **Tasks:** 2
- **Files modified:** 5 (3 created, 2 modified)

## Accomplishments

- Vendored nlohmann/json 3.12.0 (~931KB single-header) at lib/nlohmann/json.hpp for JSON serialization on both sides of the IPC boundary
- Created arch-independent IpcProtocol.hpp (MSG_PING, MSG_PONG, DEFAULT_IPC_PORT=9010) and IpcFraming.hpp (4-byte LE length-prefix encode/decode) in src/Shared/ -- no windows.h, no pch.hpp, no SDK dependencies
- Added std::mutex to PlayerRegistry with lock_guard on all 11 public methods (Init, Register, Unregister, GetByGuid, GetByIp, GetByGuidPtr, UpsertUser, InsertCharacter, GetCharactersByUserId, GetCharacterById, SetSelectedCharacter)
- Build verified clean (no mutex-related errors, only pre-existing winsock2.h ordering warning)

## Task Commits

Each task was committed atomically:

1. **Task 1: Vendor nlohmann/json and create shared protocol headers** - `48ba65f` (feat)
2. **Task 2: Add mutex to PlayerRegistry for thread safety** - `74741f0` (feat)

## Files Created/Modified

- `src/Shared/IpcProtocol.hpp` - MSG_PING/MSG_PONG constants, DEFAULT_IPC_PORT=9010, GUID format docs
- `src/Shared/IpcFraming.hpp` - 4-byte LE length-prefix Encode()/Decode() inline functions
- `lib/nlohmann/json.hpp` - nlohmann/json 3.12.0 single-header library
- `src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp` - added #include <mutex>, static std::mutex mutex_ in private section
- `src/GameServer/Storage/PlayerRegistry/PlayerRegistry.cpp` - static mutex definition, lock_guard on all 11 public methods

## Decisions Made

- Single std::mutex for entire PlayerRegistry class. by_guid_ and by_ip_ are always updated together in Register/Unregister, so per-map mutexes would require lock-ordering discipline with no benefit. Single mutex is simpler and correct.
- src/Shared/ pattern established: only standard C++ headers permitted in this directory. Enforced by arch constraint (DLL compiles as i686/Windows, control server as x86_64/Linux).
- IpcFraming byte-level encode uses char prefix array to avoid endianness issues across platforms (both are x86 little-endian but the explicit byte writes are unambiguous).

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02 (IpcClient) can now use IpcProtocol.hpp, IpcFraming.hpp, and nlohmann/json to build the ASIO-based IPC client that connects to the control server
- PlayerRegistry is safe to access from the IPC receive thread (messages pushed to inbound queue will be processed on game thread, but any future direct access from IPC thread is mutex-protected)
- lib/nlohmann/ path needs to be added to Makefile CFLAGS (-I./lib) when IpcClient sources start including json.hpp

## Self-Check: PASSED

- FOUND: src/Shared/IpcProtocol.hpp
- FOUND: src/Shared/IpcFraming.hpp
- FOUND: lib/nlohmann/json.hpp (git ls-files --others confirms untracked new file)
- FOUND: 06-01-SUMMARY.md
- FOUND: 48ba65f (Task 1 commit)
- FOUND: 74741f0 (Task 2 commit)

---
*Phase: 06-thread-safety-ipc-plumbing*
*Completed: 2026-03-21*
