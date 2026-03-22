---
phase: 10-in-game-event-routing
plan: 01
subsystem: ipc
tags: [ipc, tcp, routing, event-dispatch, inventory, quests]

# Dependency graph
requires:
  - phase: 09-instance-lifecycle-management
    provides: IpcServer, TcpSession, PlayerSessionStore, InstanceRegistry foundations
provides:
  - TcpSessionRegistry (guid -> weak_ptr<TcpSession>) for O(1) session lookup
  - MSG_GAME_EVENT/MSG_GET_RANDOMSM/MSG_RANDOMSM_RESULT IPC constants in IpcProtocol.hpp
  - QuestStore class wrapping Database quest persistence methods
  - PlayerSessionStore::Quests() static accessor to QuestStore singleton
  - IpcServer::dispatch handles GAME_EVENT and RANDOMSM_RESULT, routes to TcpSession
  - TcpSession::DeliverGameEvent dispatches spawn/beacon_pickup/beacon_remove/quest subtypes
  - TcpSession::DeliverRandomSMResult routes SM actor names to client
  - send_inventory_response sends hardcoded 7-device medic loadout matching DLL SpawnPlayerCharacter
affects: [10-02, 10-03, DLL event producers]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "TcpSessionRegistry: std::map<string, weak_ptr<TcpSession>> with mutex, registered on login, erased on disconnect"
    - "DeliverGameEvent/DeliverRandomSMResult: static lock-then-lock-weak_ptr pattern matching IpcSession g_sessions pattern"
    - "QuestStore: thin wrapper delegating to Database static methods, singleton via function-local static in PlayerSessionStore::Quests()"
    - "spawn event triggers send_inventory_response then MSG_GET_RANDOMSM IPC request to game instance"
    - "quest event: GetStatus before acting, persist before sending TCP response"

key-files:
  created:
    - src/ControlServer/QuestStore/QuestStore.hpp
    - src/ControlServer/QuestStore/QuestStore.cpp
  modified:
    - src/Shared/IpcProtocol.hpp
    - src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp
    - src/ControlServer/TcpSession/TcpSession.hpp
    - src/ControlServer/TcpSession/TcpSession.cpp
    - src/ControlServer/IpcServer/IpcServer.cpp
    - Makefile

key-decisions:
  - "Session registry lives on TcpSession (static members) not a separate class -- mirrors IpcSession g_sessions pattern, no new header needed"
  - "UnregisterSession called before ClearPendingAck on disconnect -- registry cleaned up first, ACK handlers second"
  - "send_inventory_response hardcodes 7-device loadout (not DB-driven) -- matches DLL SpawnPlayerCharacter exactly, DB-driven inventory deferred"
  - "spawn event sends MSG_GET_RANDOMSM IPC request after inventory send -- RandomSM names fetched in separate round-trip per context decision"
  - "quest GetStatus before acting: empty=accept, active=complete, complete=ignore -- replicates existing control server logic"

patterns-established:
  - "Static registry pattern: mutex + map<guid, weak_ptr> + lock/lock-weak/erase-expired in deliver methods"

requirements-completed: [EVNT-02]

# Metrics
duration: 5min
completed: 2026-03-22
---

# Phase 10 Plan 01: In-Game Event Routing Summary

**IPC GAME_EVENT routing pipeline: TcpSessionRegistry, QuestStore, DeliverGameEvent/RandomSMResult dispatch, 7-device hardcoded medic inventory replacing empty stub**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-03-22T09:29:49Z
- **Completed:** 2026-03-22T09:34:50Z
- **Tasks:** 2
- **Files modified:** 7 (2 created, 5 modified)

## Accomplishments
- Full IPC-to-TCP event routing pipeline: GAME_EVENT arrives at IpcServer, dispatched to TcpSession via guid lookup, correct send_* method called
- TcpSessionRegistry (std::map<guid, weak_ptr<TcpSession>>) with O(1) lookup, lifecycle tied to login/disconnect
- QuestStore wrapping Database quest methods with PlayerSessionStore::Quests() accessor for convenient access
- send_inventory_response upgraded from empty stub to 7-device medic loadout (5800/2991/2906/7032/2531/2773/864) matching DLL SpawnPlayerCharacter
- RANDOMSM_RESULT dispatch: IpcServer routes to TcpSession::DeliverRandomSMResult -> send_map_randomsm_settings_response

## Task Commits

Each task was committed atomically:

1. **Task 1: IPC protocol constants + QuestStore + Makefile** - `1c21558` (feat)
2. **Task 2: TcpSessionRegistry + DeliverGameEvent + IpcServer dispatch + inventory response** - `9c1ffc9` (feat)

**Plan metadata:** (docs commit follows)

## Files Created/Modified
- `src/Shared/IpcProtocol.hpp` - Added MSG_GAME_EVENT, MSG_GET_RANDOMSM, MSG_RANDOMSM_RESULT constants
- `src/ControlServer/QuestStore/QuestStore.hpp` - New: QuestStore class (GetStatus/Accept/Complete/Abandon)
- `src/ControlServer/QuestStore/QuestStore.cpp` - New: delegates to Database static methods
- `src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp` - Added Quests() inline static accessor
- `src/ControlServer/TcpSession/TcpSession.hpp` - Static registry members, DeliverGameEvent/RandomSMResult declarations, UnregisterSession on disconnect
- `src/ControlServer/TcpSession/TcpSession.cpp` - Static member defs, RegisterSession/UnregisterSession, DeliverGameEvent, DeliverRandomSMResult, 7-device send_inventory_response
- `src/ControlServer/IpcServer/IpcServer.cpp` - Added TcpSession.hpp include, GAME_EVENT and RANDOMSM_RESULT handlers in dispatch()
- `Makefile` - Added QuestStore/QuestStore.cpp to CS_CPP_SOURCES

## Decisions Made
- Session registry on TcpSession (static members) rather than a separate class -- mirrors IpcSession g_sessions pattern, avoids new header, keeps lookup logic co-located with delivery methods
- UnregisterSession before ClearPendingAck on disconnect -- prevents any in-flight GAME_EVENT from reaching a disconnecting session
- Hardcoded 7-device inventory matches DLL SpawnPlayerCharacter loadout exactly -- DB-driven per-character inventory deferred to future phase
- spawn subtype sends MSG_GET_RANDOMSM immediately after inventory -- separate IPC round-trip as decided in 10-CONTEXT.md

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Control server can route IPC GAME_EVENT to any connected TcpSession by session_guid
- DLL producers (Plan 02) can now send GAME_EVENT/RANDOMSM_RESULT IPC messages and expect correct routing
- QuestStore available for quest persistence as DLL transitions from in-process maps to IPC

## Self-Check: PASSED

- FOUND: src/ControlServer/QuestStore/QuestStore.hpp
- FOUND: src/ControlServer/QuestStore/QuestStore.cpp
- FOUND: .planning/phases/10-in-game-event-routing/10-01-SUMMARY.md
- FOUND commit 1c21558 (Task 1)
- FOUND commit 9c1ffc9 (Task 2)

---
*Phase: 10-in-game-event-routing*
*Completed: 2026-03-22*
