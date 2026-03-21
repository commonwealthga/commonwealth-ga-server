---
gsd_state_version: 1.0
milestone: v0.0.7
milestone_name: Multi-Instance Architecture
status: in_progress
last_updated: "2026-03-21"
progress:
  total_phases: 6
  completed_phases: 0
  total_plans: 1
  completed_plans: 1
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-21)

**Core value:** Players connect to a control server, select a character, and seamlessly land in a shared home map instance
**Current focus:** Phase 6 -- Thread Safety + IPC Plumbing

## Current Position

Phase: 6 of 11 (Phase 6 -- Thread Safety + IPC Plumbing)
Plan: 06-01 complete, ready for 06-02
Status: In progress
Last activity: 2026-03-21 -- 06-01 complete (IpcProtocol/IpcFraming headers, nlohmann/json vendored, PlayerRegistry mutex)

Progress: [#░░░░░░░░░] 10% (v0.0.7, 1/1 plans executed so far)

## Performance Metrics

**Velocity:**
- Total plans completed (v0.0.7): 1
- Average duration: 2 min
- Total execution time: 2 min

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

### Blockers/Concerns

- Phase 9 open question: exact Wine CLI args for per-instance port passing (env vs args vs config) -- investigate during phase 9 planning
- Phase 9 open question: UE3 startup time on this machine determines INSTANCE_READY timeout -- measure empirically during phase 9
- Note for 06-02: Makefile will need -I./lib added to CFLAGS when IpcClient sources include nlohmann/json.hpp

## Session Continuity

Last session: 2026-03-21
Stopped at: Completed 06-01 (IPC foundation + PlayerRegistry mutex). Ready for 06-02 (IpcClient ASIO thread).
Resume file: None

---
*Last updated: 2026-03-21 after 06-01 completion*
