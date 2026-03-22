---
gsd_state_version: 1.0
milestone: v0.0
milestone_name: milestone
status: unknown
last_updated: "2026-03-22T09:36:11.793Z"
progress:
  total_phases: 6
  completed_phases: 5
  total_plans: 14
  completed_plans: 13
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-21)

**Core value:** Players connect to a control server, select a character, and seamlessly land in a shared home map instance
**Current focus:** Phase 10 -- In-Game Event Routing

## Current Position

Phase: 10 of 11 (Phase 10 -- In-Game Event Routing)
Plan: 10-01 complete (1/2 plans executed in phase 10)
Status: Phase 10 in progress
Last activity: 2026-03-22 -- 10-01 complete (TcpSessionRegistry, GAME_EVENT dispatch, 7-device inventory, QuestStore)

Progress: [#########░] 87% (v0.0.10, 1/2 plans of phase 10 done)

## Performance Metrics

**Velocity:**
- Total plans completed (v0.0.7): 3
- Average duration: 6.7 min
- Total execution time: 18 min

| Phase | Plan | Duration | Tasks | Files |
|-------|------|----------|-------|-------|
| 06    | 01   | ~4 min   | 3     | 3     |
| 06    | 02   | ~5 min   | 4     | 8     |
| 07    | 01   | ~9 min   | 3     | 13    |
| 07    | 02   | ~9 min   | 2     | 5     |
| 07    | 03   | ~12 min  | 3     | 5     |
| 08    | 01   | ~4 min   | 2     | 7     |
| 08    | 02   | ~5 min   | 2     | 4     |
| 08    | 03   | ~3 min   | 2     | 2     |
| 09    | 01   | ~3 min   | 2     | 9     |
| 09    | 02   | ~8 min   | 2     | 6     |
| 09    | 03   | ~10 min  | 2     | 8     |

*Updated after each plan completion*
| Phase 10-in-game-event-routing P01 | 5 | 2 tasks | 7 files |

## Accumulated Context

### Decisions

- v0.0.7: TCP sockets for IPC (works across Wine boundary, extends to multi-machine)
- v0.0.7: Control server owns all client TCP connections (clean separation: control = TCP + state, game = UDP + gameplay)
- v0.0.7: Session GUIDs are canonical cross-process identity (never raw UNetConnection pointers)
- v0.0.7: IPC sends from game thread must be enqueued (dedicated sender thread drains; never synchronous send on UE3 tick)
- v0.0.7: PlayerRegistry mutex required before any IPC work (race is latent, becomes certain crash with second thread)
- v0.0.7: Shared home map instances (one UE3 process per map, pre-spawned at startup)
- 06-01: Single std::mutex for all PlayerRegistry maps (by_guid_ and by_ip_ always updated together)
- 06-01: src/Shared/ pattern established -- only standard C++ headers, compiles on i686 (DLL) and x86_64 (control server)
- 06-01: 4-byte LE framing for IPC matches existing TcpSession convention
- 06-02: CRITICAL_SECTION used instead of std::mutex for IpcClient -- MinGW win32 threading model lacks <mutex>
- 06-02: write_in_progress chain pattern -- only one async_write in flight at a time on the ASIO thread
- 06-02: asio::post() used from game thread Send() to safely kick writes to ASIO thread
- 06-02: Actor__Tick hook activated -- game thread now drains IPC inbound queue each tick
- 07-01: sqlite3.c compiled with gcc (not g++) -- g++ void* strictness breaks sqlite3 amalgamation; separate .o file
- 07-01: SessionInfo drops player_name_w (wstring) -- control server has no UE3 SDK or Win32 wide-char dependency
- 07-01: std::mutex used for PlayerSessionStore (native Linux has std::mutex unlike i686 MinGW)
- 07-01: Database.cpp does not call PlayerSessionStore::Init() -- called separately from main.cpp
- [Phase 07-02]: Sleep(1000) replaced with asio::steady_timer (non-blocking, io_context-safe)
- [Phase 07-02]: send_inventory_response stubbed with empty response for Phase 10 IPC
- [Phase 07-02]: Config::GetIpChar/GetPort hardcoded to 127.0.0.1:9002 with Phase 8 TODO
- [Phase 07-02]: RELAY_LOG handler stripped of DLL event maps, Phase 10 IPC stub comment left in place
- [Phase 07]: IpcSession defined in IpcServer.cpp (not .hpp) -- private to TU, no external linkage needed
- [Phase 07]: write_in_progress chain for IpcSession -- single-threaded ASIO ops, no strand needed, same pattern as IpcClient
- [Phase 07]: Phase 10 stub comment in IpcServer::dispatch() marks where GAME_EVENT/PLAYER_SPAWNED handlers will be added
- [Phase 08-01]: InstanceRegistry::Init() is a no-op (mutex default-constructed) for symmetry with PlayerSessionStore pattern
- [Phase 08-01]: SeedHomeMapInstance stops all non-STOPPED rows before inserting fresh READY row (crash-recovery safe)
- [Phase 08-01]: --game-port default 9002 matches Config.cpp hardcoded UDP port from Phase 07-02
- [Phase 08-02]: DeliverAck public in IpcServer so IpcSession (same TU) can call it without friend declaration
- [Phase 08-02]: Both ADD_PLAYER_CHARACTER branches collapsed to single initiate_player_register_and_go_play() call
- [Phase 08-03]: morph_data not stored in PlayerInfo at IPC registration -- SpawnPlayerCharacter reads from DLL sqlite DB
- [Phase 08-03]: pawn_id=0 in PLAYER_REGISTER_ACK -- pawn spawns later at UDP JOIN, not at IPC registration time
- [Phase 08-03]: Unregistered UDP players (no PLAYER_REGISTER) allowed through with log warning for direct-connect testing
- [Phase 09-01]: rowid-as-instance-id -- INSERT then UPDATE instance_id=rowid; avoids UUID, stable within DB lifetime
- [Phase 09-01]: AllocatePort holds mutex for full read-then-decide to avoid TOCTOU between port check and return
- [Phase 09-01]: ClearStaleInstances at startup -- crash-recovery pattern, ensures no STARTING/READY rows survive a restart
- [Phase 09-01]: InstanceSpawner stub added to Makefile in Plan 01 so Plan 02 only needs to create the .cpp file
- [Phase 09-02]: g_sessions/sessions_mutex_ on IpcSession (same TU) -- avoids IpcServer.hpp pollution, accessed via friend
- [Phase 09-02]: on_disconnect() factored out of read error paths -- single place for g_sessions cleanup and MarkStopped
- [Phase 09-02]: main.cpp config-only -- all flags consolidated to --config=PATH; JSON config drives all values
- [Phase 09-02]: TcpSession routes PLAYER_REGISTER via GetReadyHomeInstance().instance_id -- single home map assumption for Phase 9
- [Phase 09-03]: max_players hardcoded to 10 in INSTANCE_READY for Phase 9; read from GameInfo in future phase
- [Phase 09-03]: wait_for_home_map_then_register uses 2s poll interval, 120s timeout; client stays on loading screen silently on timeout
- [Phase 09-03]: assigned_instance_id_ cached on TcpSession when home map found -- prevents TOCTOU between find and route
- [Phase 10-01]: Session registry on TcpSession static members -- mirrors IpcSession g_sessions pattern, keeps lookup co-located with delivery
- [Phase 10-01]: Hardcoded 7-device medic inventory in send_inventory_response -- matches DLL SpawnPlayerCharacter exactly, DB-driven deferred
- [Phase 10-01]: spawn GAME_EVENT sends MSG_GET_RANDOMSM immediately after inventory -- separate IPC round-trip per 10-CONTEXT decision

### Blockers/Concerns

- Phase 9 open question: exact Wine CLI args for per-instance port passing (env vs args vs config) -- investigate during phase 9 planning
- Phase 9 open question: UE3 startup time on this machine determines INSTANCE_READY timeout -- measure empirically during phase 9
- Note resolved: nlohmann json.hpp included via direct path `lib/nlohmann/json.hpp` (no -I flag needed, -I. already covers it)

## Session Continuity

Last session: 2026-03-22
Stopped at: Completed 10-01 (TcpSessionRegistry, GAME_EVENT dispatch, 7-device inventory response, QuestStore).
Resume file: None

---
*Last updated: 2026-03-22 after 10-01 completion*
