---
phase: 09-instance-lifecycle-management
plan: 03
subsystem: infra
tags: [ipc, instance-lifecycle, asio, cpp, game-server]

# Dependency graph
requires:
  - phase: 09-01
    provides: InstanceRegistry with GetReadyHomeInstance, IpcProtocol MSG_INSTANCE_HELLO/ACK/READY
  - phase: 09-02
    provides: IpcServer SendToInstance(instance_id), multi-session g_sessions map, INSTANCE_HELLO/READY handlers
provides:
  - DLL reads -instanceid from CLI and sends INSTANCE_HELLO with instance_id on every IPC connect
  - DLL sends INSTANCE_READY with instance_id and max_players after World::BeginPlay
  - TcpSession waits for READY home map instance (2s poll, 120s timeout) before routing player
  - PLAYER_REGISTER routed to specific instance_id via SendToInstance
  - send_go_play_response and send_go_play_tutorial_response use GetReadyHomeInstance
affects:
  - phase-10-game-events
  - phase-11-multi-instance

# Tech tracking
tech-stack:
  added: []
  patterns:
    - ASIO steady_timer recursive poll pattern for readiness waiting
    - assigned_instance_id_ cached on TcpSession for per-session routing

key-files:
  created: []
  modified:
    - src/Config/Config.hpp
    - src/Config/Config.cpp
    - src/IpcClient/IpcClient.hpp
    - src/IpcClient/IpcClient.cpp
    - src/GameServer/Engine/World/BeginPlay/World__BeginPlay.cpp
    - src/dllmain.cpp
    - src/ControlServer/TcpSession/TcpSession.hpp
    - src/ControlServer/TcpSession/TcpSession.cpp

key-decisions:
  - "max_players hardcoded to 10 in INSTANCE_READY for Phase 9; read from GameInfo in future phase"
  - "wait_for_home_map_then_register uses 2s poll interval with 120s total timeout; client stays on loading screen on timeout"
  - "assigned_instance_id_ cached on TcpSession when home map found, not re-looked up in initiate_player_register_and_go_play"

patterns-established:
  - "ASIO steady_timer recursive countdown: wait_for_home_map_then_register(remaining_seconds - 2) pattern for polling without blocking"

requirements-completed:
  - INST-03
  - INST-05

# Metrics
duration: 10min
completed: 2026-03-22
---

# Phase 9 Plan 3: Instance Lifecycle Management - DLL Signaling and TcpSession Routing Summary

**DLL sends INSTANCE_HELLO on IPC connect and INSTANCE_READY after map load; TcpSession polls for READY home map before routing player, closing the Phase 9 instance lifecycle loop**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-03-22T00:15:00Z
- **Completed:** 2026-03-22T00:25:00Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- DLL reads -instanceid from CLI args via Config::GetInstanceId() and passes it through IpcClient::Init()
- IpcClient sends INSTANCE_HELLO with instance_id after every successful IPC connect (reconnects included)
- IpcClient handles INSTANCE_HELLO_ACK by logging accepted flag
- World::BeginPlay sends INSTANCE_READY with instance_id and max_players=10 after map load
- TcpSession::wait_for_home_map_then_register() polls GetReadyHomeInstance() every 2 seconds with 120-second timeout
- send_go_play_response and send_go_play_tutorial_response switched from GetReadyInstance("Rot_Redistribution05") to GetReadyHomeInstance()
- Second player connecting reuses same READY home map instance (INST-05 satisfied)

## Task Commits

Each task was committed atomically:

1. **Task 1: DLL-side INSTANCE_HELLO and INSTANCE_READY signaling** - `5091242` (feat)
2. **Task 2: TcpSession readiness wait + instance_id routing** - `96ca6d2` (feat)

## Files Created/Modified

- `src/Config/Config.hpp` - Added GetInstanceId() declaration
- `src/Config/Config.cpp` - Implemented GetInstanceId() reading -instanceid CLI switch
- `src/IpcClient/IpcClient.hpp` - Added instance_id_ member, updated Init() signature
- `src/IpcClient/IpcClient.cpp` - Added instance_id_ definition, updated Init(), sends INSTANCE_HELLO on connect, handles INSTANCE_HELLO_ACK
- `src/GameServer/Engine/World/BeginPlay/World__BeginPlay.cpp` - Sends INSTANCE_READY after DumpKismetSequence
- `src/dllmain.cpp` - Passes Config::GetInstanceId() to IpcClient::Init()
- `src/ControlServer/TcpSession/TcpSession.hpp` - Added assigned_instance_id_ and wait_for_home_map_then_register() declaration
- `src/ControlServer/TcpSession/TcpSession.cpp` - Implemented wait_for_home_map_then_register(), updated SELECT_CHARACTER and ADD_PLAYER_CHARACTER handlers, switched go_play functions to GetReadyHomeInstance()

## Decisions Made

- max_players hardcoded to 10 in INSTANCE_READY for Phase 9; will be read from GameInfo in a future phase when GameInfo replication is handled
- wait_for_home_map_then_register uses 2s poll interval, 120s total timeout; on timeout client stays on loading screen silently (same behavior as pre-existing PLAYER_REGISTER timeout)
- assigned_instance_id_ is cached on TcpSession when the home map is found, so initiate_player_register_and_go_play does not re-query the registry (prevents TOCTOU between find and route)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 9 instance lifecycle loop is complete: control server spawns instance, instance signals HELLO and READY, TcpSession waits for READY before routing player
- Phase 10 can now focus on game event IPC (GAME_EVENT, PLAYER_SPAWNED) using the established READY instance routing
- PLAYER_REGISTER is reliably routed to a specific instance_id; Phase 10 game events will use the same SendToInstance pattern

---
*Phase: 09-instance-lifecycle-management*
*Completed: 2026-03-22*

## Self-Check: PASSED

- FOUND: src/Config/Config.hpp
- FOUND: src/IpcClient/IpcClient.cpp
- FOUND: src/ControlServer/TcpSession/TcpSession.cpp
- FOUND: .planning/phases/09-instance-lifecycle-management/09-03-SUMMARY.md
- FOUND commit: 5091242 (feat(09-03): DLL INSTANCE_HELLO/READY signaling)
- FOUND commit: 96ca6d2 (feat(09-03): TcpSession home map readiness wait)
- control-server-linux build: exit 0
