---
phase: 10-in-game-event-routing
plan: 02
subsystem: ipc
tags: [ipc, json, nlohmann, game-events, beacon, quest, randomsm]

# Dependency graph
requires:
  - phase: 10-01
    provides: TcpSessionRegistry, GAME_EVENT dispatch infrastructure in control server, IpcProtocol MSG_GAME_EVENT/MSG_GET_RANDOMSM/MSG_RANDOMSM_RESULT constants
  - phase: 06-02
    provides: IpcClient Send/DrainInbound pattern, CRITICAL_SECTION outbound queue
provides:
  - "IpcClient::GetInstanceId() public accessor"
  - "Spawn event as GAME_EVENT subtype=spawn via IpcClient::Send (replaces GTcpEvents write)"
  - "Beacon pickup event as GAME_EVENT subtype=beacon_pickup via IpcClient::Send (replaces GBeaconPickupEvents write)"
  - "Beacon remove event as GAME_EVENT subtype=beacon_remove via IpcClient::Send (replaces GBeaconRemoveEvents write)"
  - "Quest event as GAME_EVENT subtype=quest via IpcClient::Send (replaces GQuestEvents write)"
  - "DrainInbound MSG_GET_RANDOMSM handler: iterates GObjObjects, collects enabled actor names via UObject__GetName, sends MSG_RANDOMSM_RESULT"
affects: [11-cleanup, control-server-10-01-handlers]

# Tech tracking
tech-stack:
  added: []
  patterns: [comment-out-dont-delete for retired global map writes]

key-files:
  created: []
  modified:
    - src/IpcClient/IpcClient.hpp
    - src/IpcClient/IpcClient.cpp
    - src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp
    - src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.cpp
    - src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.cpp
    - src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.cpp

key-decisions:
  - "GetInstanceId() inline in header -- zero overhead, no new .cpp linkage needed"
  - "Old global map writes commented out, not deleted -- Phase 11 CLEN-01 will remove TcpEvents.hpp entirely"
  - "GPawnSessions write preserved in spawn producer -- beacon/quest hooks still resolve session locally from pawn pointer"
  - "MSG_GET_RANDOMSM uses ATgRandomSMActor::StaticClass() (available via pch.hpp SDK) rather than GObjObjects index fallback"

patterns-established:
  - "Comment-out pattern: retired event writes wrapped in // [Phase 10] Replaced comments for Phase 11 cleanup"

requirements-completed: [EVNT-01, EVNT-02]

# Metrics
duration: 8min
completed: 2026-03-22
---

# Phase 10 Plan 02: In-Game Event Routing (DLL Side) Summary

**All 4 DLL event producers switched from global maps to GAME_EVENT IPC; DrainInbound handles MSG_GET_RANDOMSM with actor name collection**

## Performance

- **Duration:** 8 min
- **Started:** 2026-03-22T09:36:11Z
- **Completed:** 2026-03-22T09:47:52Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- Added `IpcClient::GetInstanceId()` public static accessor returning the instance_id_ stored at Init
- Replaced all 4 global map event writes with `IpcClient::Send(ev.dump())` GAME_EVENT IPC messages: spawn (MarshalChannel__NotifyControlMessage), beacon_pickup (NonPersistAddDevice), beacon_remove (NonPersistRemoveDevice), quest (CGameClient__MarshalReceived)
- GPawnSessions reverse map write preserved in spawn producer -- local beacon/quest resolution still depends on it
- DrainInbound now handles MSG_GET_RANDOMSM: iterates GObjObjects for enabled TgRandomSMActors (bCollideActors=true at 0xB0 bit 0x08000000), gets full names via UObject__GetName (0x1090d1d0), frees FString via appFree (0x112c18c0), sends MSG_RANDOMSM_RESULT with names[] array
- DLL compiles cleanly on both targets (out/version.dll and out/client/version.dll)

## Task Commits

Each task was committed atomically:

1. **Task 1: GetInstanceId accessor + switch all 4 event producers to IPC** - `be01397` (feat)
2. **Task 2: DrainInbound MSG_GET_RANDOMSM handler** - `8739d36` (feat)

## Files Created/Modified
- `src/IpcClient/IpcClient.hpp` - Added GetInstanceId() public static accessor
- `src/IpcClient/IpcClient.cpp` - Added MSG_GET_RANDOMSM handler in DrainInbound with GObjObjects iteration, UObject__GetName, appFree, MSG_RANDOMSM_RESULT response
- `src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp` - Commented out GTcpEvents write; added GAME_EVENT subtype=spawn with instance_id/session_guid/pawn_id
- `src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.cpp` - Commented out GBeaconPickupEvents write; added GAME_EVENT subtype=beacon_pickup with device_id/inventory_id/equip_slot_value_id
- `src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.cpp` - Commented out GBeaconRemoveEvents write; added GAME_EVENT subtype=beacon_remove with pawn_id/inventory_id
- `src/GameServer/Misc/CGameClient/MarshalReceived/CGameClient__MarshalReceived.cpp` - Commented out GQuestEvents write; added GAME_EVENT subtype=quest with quest_id/abandon/character_id

## Decisions Made
- GetInstanceId() inline in header -- zero overhead, no new .cpp linkage needed
- Old global map writes commented out, not deleted -- Phase 11 CLEN-01 will remove TcpEvents.hpp entirely
- GPawnSessions write preserved in spawn producer -- beacon/quest hooks still resolve session locally from pawn pointer
- MSG_GET_RANDOMSM uses ATgRandomSMActor::StaticClass() (available via pch.hpp SDK) rather than GObjObjects index fallback

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- DLL side of EVNT-01/EVNT-02 complete
- Control server already has GAME_EVENT dispatch infrastructure from Plan 01 (TcpSessionRegistry, IpcServer GAME_EVENT handler with phase-10-01 placeholder resolved)
- Phase 11 CLEN-01 can now remove TcpEvents.hpp and the global map declarations (GTcpEvents, GBeaconPickupEvents, GBeaconRemoveEvents, GQuestEvents)

---
*Phase: 10-in-game-event-routing*
*Completed: 2026-03-22*
