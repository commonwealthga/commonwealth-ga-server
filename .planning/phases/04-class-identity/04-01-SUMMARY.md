---
phase: 04-class-identity
plan: 01
subsystem: gameserver
tags: [spawn, identity, profileId, skillGroupSetId, ClassConfig, InitializeDefaultProps]

# Dependency graph
requires:
  - phase: 03-refactor-existing-code
    provides: Inventory::Equip API and unified ID counter used in SpawnPlayerCharacter
provides:
  - ClassConfig struct with per-class identity and device loadout fields (GameTypes.h)
  - GetClassConfig(profileId) lookup function — single source of truth for all 4 classes
  - SpawnPlayerCharacter reads selected_profile_id and sets Pawn/PRI identity fields from ClassConfig
  - Pawn HP and power pool derived from InitializeDefaultProps DB output instead of hardcoded values
  - TcpSession.send_character_list_response uses GetClassConfig instead of local lambda
affects: [05-class-equipment, SpawnPlayerCharacter, TcpSession]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - ClassConfig struct as per-class configuration table (identity + Phase 5 equipment IDs)
    - GetClassConfig(profileId) free function for O(1) class lookup with Assault default fallback
    - nPendingBotId = profileId pattern: bot_id == profile_id for player classes in DB

key-files:
  created: []
  modified:
    - src/GameServer/Constants/GameTypes.h
    - src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp
    - src/TcpServer/TcpSession/TcpSession.cpp

key-decisions:
  - "ClassConfig includes Phase 5 device ID fields now so GetClassConfig is already the loadout source for Phase 5"
  - "nPendingBotId = profileId — player classes use profile_id as their bot_id in asm_data_set_bots"
  - "HP/power derived from InitializeDefaultProps output (HealthMax, r_fMaxPowerPool) not hardcoded; class stats come from DB"
  - "GetClassConfig defaults to Assault (680) for unknown profile IDs — explicit fallback with warning log"

patterns-established:
  - "GetClassConfig(profileId): single dispatch point for all per-class constants, used in both TCP and game layers"
  - "Read-back pattern: call SetProperty via InitializeDefaultProps, then read Pawn->HealthMax after Spawn()"

requirements-completed: [CLID-01, CLID-02, CLID-03, CLID-04, CLID-05]

# Metrics
duration: 12min
completed: 2026-03-21
---

# Phase 4 Plan 01: Class Identity Summary

**ClassConfig struct + GetClassConfig lookup in GameTypes.h; SpawnPlayerCharacter reads selected_profile_id and derives Pawn/PRI identity and HP/power from per-class DB config instead of medic hardcodes**

## Performance

- **Duration:** 12 min
- **Started:** 2026-03-21T00:00:00Z
- **Completed:** 2026-03-21T00:12:00Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- ClassConfig struct with profileId, skillGroupSetId, and 8 device ID fields for all 4 classes added to GameTypes.h
- GetClassConfig(profileId) free function returns correct config per class, defaults to Assault for unknown IDs
- SpawnPlayerCharacter reads selected_profile_id from GClientConnectionsData, validates it, sets nPendingBotId=profileId before Spawn() so InitializeDefaultProps loads the correct class stats from DB
- Pawn and PRI r_nProfileId, r_nSkillGroupSetId, s_nCharacterId, r_nHealthCurrent/Maximum, and team entry health all derive from classConfig or InitializeDefaultProps output — no more hardcoded 567/1300/100
- TcpSession.send_character_list_response replaces local skillGroupSetId lambda with GetClassConfig call

## Task Commits

Each task was committed atomically:

1. **Task 1: Add ClassConfig struct and GetClassConfig to GameTypes.h** - `c6b234e` (feat)
2. **Task 2: Make SpawnPlayerCharacter class-aware and consolidate TcpSession helper** - `4b6200a` (feat)

## Files Created/Modified
- `src/GameServer/Constants/GameTypes.h` - Added ClassConfig struct and GetClassConfig inline function; includes DeviceIds.hpp for GA::DeviceId:: constants
- `src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp` - Class-aware spawn: reads selected_profile_id, validates, uses classConfig for all identity fields, reads HP/power from InitializeDefaultProps
- `src/TcpServer/TcpSession/TcpSession.cpp` - Replaced local skillGroupSetId lambda with GetClassConfig(c.profile_id).skillGroupSetId

## Decisions Made
- ClassConfig includes Phase 5 device loadout fields now (meleeDeviceId through moraleDeviceId) so the struct is the single authoritative source for Phase 5 without needing another table
- nPendingBotId = profileId because asm_data_set_bots stores player class configs keyed by profile_id (567=Medic, 680=Assault, etc.)
- HP/power uses read-back pattern: InitializeDefaultProps::Call sets Pawn->HealthMax via SetProperty(TGPID_HEALTH_MAX), then SpawnPlayerCharacter reads back `(int)newpawn->HealthMax` after Spawn() returns

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 5 (class equipment) can consume `classConfig.meleeDeviceId`, `classConfig.rangedDeviceId`, etc. directly from GetClassConfig — the loadout table is already populated
- The TODO(Phase 5) comment in SpawnPlayerCharacter marks the exact location to replace the hardcoded medic equip block
- Build confirmed clean (no errors, only pre-existing winsock2.h include-order warning)

## Self-Check: PASSED

- src/GameServer/Constants/GameTypes.h — FOUND
- src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp — FOUND
- src/TcpServer/TcpSession/TcpSession.cpp — FOUND
- .planning/phases/04-class-identity/04-01-SUMMARY.md — FOUND
- commit c6b234e — FOUND
- commit 4b6200a — FOUND

---
*Phase: 04-class-identity*
*Completed: 2026-03-21*
