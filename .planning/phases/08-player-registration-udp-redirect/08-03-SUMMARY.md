---
phase: 08-player-registration-udp-redirect
plan: "03"
subsystem: ipc
tags: [ipc, player-register, player-registry, profile-id, spawn-class]
dependency_graph:
  requires:
    - phase: 08-01
      provides: IpcProtocol.MSG_PLAYER_REGISTER / MSG_PLAYER_REGISTER_ACK constants
  provides:
    - IpcClient.DrainInbound PLAYER_REGISTER handler (populates PlayerRegistry + sends ACK)
    - JOIN handler copies selected_profile_id and selected_character_id to GClientConnectionsData
  affects: [src/IpcClient/IpcClient.cpp, src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp]
tech_stack:
  added: []
  patterns: [DrainInbound message dispatch (else-if chain), PlayerRegistry pre-population via IPC before UDP JOIN]
key_files:
  created: []
  modified:
    - src/IpcClient/IpcClient.cpp
    - src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp
key_decisions:
  - "morph_data not stored in PlayerInfo at registration -- SpawnPlayerCharacter reads from DLL sqlite DB directly"
  - "Unregistered UDP players (no PLAYER_REGISTER) are allowed with a log warning -- direct-connect testing still works"
  - "pawn_id=0 in PLAYER_REGISTER_ACK -- pawn not yet spawned at registration time (spawns at UDP JOIN)"
requirements-completed: [ROUT-04]
duration: ~3min
completed: "2026-03-22"
---

# Phase 8 Plan 3: DLL PLAYER_REGISTER Handler and Profile ID Fix Summary

**PLAYER_REGISTER IPC handler in IpcClient::DrainInbound + JOIN handler profile_id copy closing the SpawnPlayerCharacter class selection gap (ROUT-04)**

## Performance

- **Duration:** ~3 min
- **Started:** 2026-03-22
- **Completed:** 2026-03-22
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- DLL IpcClient::DrainInbound now handles MSG_PLAYER_REGISTER by populating PlayerRegistry with session_guid, player_name, user_id, selected_character_id, and selected_profile_id before any UDP connection arrives
- Sends MSG_PLAYER_REGISTER_ACK (success=true, pawn_id=0) back to control server immediately after registration, completing the IPC round-trip
- JOIN handler in MarshalChannel__NotifyControlMessage now copies selected_profile_id and selected_character_id from PlayerRegistry to GClientConnectionsData so SpawnPlayerCharacter reads the correct class identity (Assault/Medic/Recon/Robo) for IPC-registered players
- Direct-connect players (no prior PLAYER_REGISTER) still work with a log warning

## Task Commits

1. **Task 1: DLL IpcClient PLAYER_REGISTER handler + ACK response** - `6e72538` (feat)
2. **Task 2: JOIN handler copies profile_id and character_id from PlayerRegistry** - `5a3e21b` (feat)

## Files Created/Modified

- `src/IpcClient/IpcClient.cpp` - Added MSG_PLAYER_REGISTER handler in DrainInbound; added includes for PlayerRegistry.hpp and HexUtils.hpp
- `src/GameServer/TgNetDrv/MarshalChannel/NotifyControlMessage/MarshalChannel__NotifyControlMessage.cpp` - JOIN handler now copies selected_profile_id and selected_character_id; updated log messages

## Decisions Made

- morph_data is included in the PLAYER_REGISTER payload but not stored in PlayerInfo -- SpawnPlayerCharacter reads morph_data from the DLL's own sqlite DB (populated at character creation time via old flow). Avoids bloating PlayerInfo with fields only one consumer needs.
- Unregistered players are allowed through with a log warning for pragmatic direct-connect testing without a full control server flow.
- pawn_id=0 in the ACK because the pawn hasn't been spawned yet -- it spawns later when the UDP JOIN arrives. The ACK only confirms the registry entry exists.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 8 is now complete: control server sends PLAYER_REGISTER via IPC before redirecting the client to UDP, the DLL registers the player and ACKs, and the JOIN handler uses the registered profile_id to spawn the correct class.
- Phase 9 (instance lifecycle management) can proceed: the PLAYER_REGISTER / ACK round-trip is fully operational end-to-end.

---
*Phase: 08-player-registration-udp-redirect*
*Completed: 2026-03-22*
