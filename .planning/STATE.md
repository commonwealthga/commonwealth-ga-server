---
gsd_state_version: 1.0
milestone: v0.0
milestone_name: milestone
status: unknown
last_updated: "2026-03-22T00:08:49.917Z"
progress:
  total_phases: 3
  completed_phases: 3
  total_plans: 6
  completed_plans: 6
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-21)

**Core value:** Players connect to a control server, select a character, and seamlessly land in a shared home map instance
**Current focus:** Phase 7 -- Control Server Protocol Migration

## Current Position

Phase: 7 of 11 (Phase 7 -- Control Server Protocol Migration)
Plan: 07-03 complete
Status: In progress
Last activity: 2026-03-22 -- 07-03 complete (IpcServer, main.cpp wiring, DLL TCP cutover -- Phase 7 done)

Progress: [###░░░░░░] 30% (v0.0.7, 4/3 plans executed in phase 7)

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

*Updated after each plan completion*

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

### Blockers/Concerns

- Phase 9 open question: exact Wine CLI args for per-instance port passing (env vs args vs config) -- investigate during phase 9 planning
- Phase 9 open question: UE3 startup time on this machine determines INSTANCE_READY timeout -- measure empirically during phase 9
- Note resolved: nlohmann json.hpp included via direct path `lib/nlohmann/json.hpp` (no -I flag needed, -I. already covers it)

## Session Continuity

Last session: 2026-03-22
Stopped at: Completed 07-03 (IpcServer, main.cpp wiring, DLL TCP cutover -- Phase 7 complete).
Resume file: None

---
*Last updated: 2026-03-22 after 07-03 completion*
