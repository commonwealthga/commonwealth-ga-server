---
phase: 08-player-registration-udp-redirect
plan: "02"
subsystem: control-server
tags: [ipc, player-register, ack-wait, instance-registry, tcp-session]
dependency_graph:
  requires: [08-01]
  provides: [IpcServer.RegisterPendingAck, IpcServer.SendToInstance, TcpSession.initiate_player_register_and_go_play]
  affects: [src/ControlServer/IpcServer/IpcServer.hpp, src/ControlServer/IpcServer/IpcServer.cpp, src/ControlServer/TcpSession/TcpSession.hpp, src/ControlServer/TcpSession/TcpSession.cpp]
tech_stack:
  added: []
  patterns: [pending ACK map (mutex + std::map<guid, callback>), weak_ptr IpcSession tracking, asio::steady_timer 60s timeout, shared_from_this capture in async callbacks]
key_files:
  created: []
  modified:
    - src/ControlServer/IpcServer/IpcServer.hpp
    - src/ControlServer/IpcServer/IpcServer.cpp
    - src/ControlServer/TcpSession/TcpSession.hpp
    - src/ControlServer/TcpSession/TcpSession.cpp
decisions:
  - DeliverAck moved to public section of IpcServer so IpcSession (same TU, private class) can call it without a friend declaration
  - trainingMapGameId commented out rather than deleted -- both ADD_PLAYER_CHARACTER branches now call initiate_player_register_and_go_play regardless
  - send_go_play_tutorial_response now queries InstanceRegistry just like send_go_play_response; tutorial path is effectively identical to normal path
metrics:
  duration: "~5 min"
  completed_date: "2026-03-22"
  tasks_completed: 2
  files_changed: 4
---

# Phase 8 Plan 2: PLAYER_REGISTER / ACK Flow Summary

PLAYER_REGISTER IPC flow from control server to game instance: pending ACK map in IpcServer, 60-second timer in TcpSession replacing 1-second blind delay, and InstanceRegistry-driven go_play address.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | IpcServer pending ACK map + IpcSession tracking + PLAYER_REGISTER_ACK dispatch | 9889734 | IpcServer.hpp, IpcServer.cpp |
| 2 | TcpSession ACK-wait flow + go_play registry lookup + disconnect cleanup | bc41594 | TcpSession.hpp, TcpSession.cpp |

## What Was Built

### IpcServer (extended)

`RegisterPendingAck(guid, callback)`: stores callback in `pending_acks_` map under `ack_mutex_`.

`ClearPendingAck(guid)`: erases entry (called on timeout or disconnect).

`SendToInstance(json_msg)`: locks `g_active_ipc_session` weak_ptr, calls `session->send()`. Returns false if no game instance is connected.

`DeliverAck(guid, success, pawn_id)`: extracts + erases callback under lock, invokes it outside the lock. Called from `IpcSession::dispatch()` on `MSG_PLAYER_REGISTER_ACK`.

`g_active_ipc_session` (file-scope weak_ptr): set in `do_accept()` when a game instance connects. Phase 9 will expand to per-instance routing.

### TcpSession (extended)

`initiate_player_register_and_go_play()`: builds PLAYER_REGISTER JSON (session_guid, profile_id, player_name, user_id, character_id, task_force=1, head_asm_id, gender_type_value_id, morph_data hex-encoded), arms 60-second `asio::steady_timer`, registers ACK callback via `IpcServer::RegisterPendingAck`, then calls `IpcServer::SendToInstance`. On ACK success: cancels timer, calls `send_go_play_response()`. On 60-second timeout: calls `IpcServer::ClearPendingAck`, silent failure. If `SendToInstance` fails: immediately cancels timer and clears pending ACK.

`GSC_SELECT_CHARACTER`: 1-second blind timer removed, replaced with `initiate_player_register_and_go_play()`.

`ADD_PLAYER_CHARACTER`: both tutorial and normal branches collapsed to `initiate_player_register_and_go_play()`.

`do_read() disconnect handler`: cancels `pending_ack_timer_`, calls `IpcServer::ClearPendingAck` before `PlayerSessionStore::Unregister`.

`send_go_play_response()`: calls `InstanceRegistry::GetReadyInstance("Rot_Redistribution05")`, uses `instance->ip_address` and `instance->udp_port` for `WriteIP`. Returns early if no READY instance.

`send_go_play_tutorial_response()`: same InstanceRegistry lookup pattern.

## Verification Results

1. `make control-server-linux` -- PASS (no warnings, no errors)
2. No hardcoded 9002 in `send_go_play_response` or `send_go_play_tutorial_response` -- PASS
3. `IpcServer.hpp` exports `RegisterPendingAck`, `ClearPendingAck`, `SendToInstance` -- PASS
4. `TcpSession.hpp` has `pending_ack_timer_` member and `initiate_player_register_and_go_play` declaration -- PASS
5. `TcpSession.cpp` `GSC_SELECT_CHARACTER` calls `initiate_player_register_and_go_play` -- PASS
6. `TcpSession.cpp` `do_read` error handler cancels `pending_ack_timer_` -- PASS

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] DeliverAck access from IpcSession**
- **Found during:** Task 1
- **Issue:** `IpcSession::dispatch()` calls `IpcServer::DeliverAck()` which was declared private. `friend class IpcServer` in IpcSession does not help because we need the reverse friendship.
- **Fix:** Moved `DeliverAck` to public section of `IpcServer` with a comment noting it is called from IpcSession (same TU). The method is not callable from outside the TU in any useful way regardless.
- **Files modified:** src/ControlServer/IpcServer/IpcServer.hpp
- **Commit:** 9889734 (integrated into Task 1 commit)

**2. [Rule 1 - Bug] trainingMapGameId unused warning**
- **Found during:** Task 2
- **Issue:** After merging both ADD_PLAYER_CHARACTER branches into a single `initiate_player_register_and_go_play()` call, `trainingMapGameId` became an unused variable causing a -Wall warning.
- **Fix:** Commented out variable declaration per project policy; replaced with `pkt.Read4B(...)` to consume the field without storing it.
- **Files modified:** src/ControlServer/TcpSession/TcpSession.cpp
- **Commit:** bc41594 (integrated into Task 2 commit)

## Self-Check

### Files exist:
- src/ControlServer/IpcServer/IpcServer.hpp: FOUND
- src/ControlServer/IpcServer/IpcServer.cpp: FOUND
- src/ControlServer/TcpSession/TcpSession.hpp: FOUND
- src/ControlServer/TcpSession/TcpSession.cpp: FOUND

### Commits exist:
- 9889734: FOUND
- bc41594: FOUND

## Self-Check: PASSED
