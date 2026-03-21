---
phase: 01-enums-and-constants
plan: 01
subsystem: api
tags: [constants, enums, sqlite3, inventory, equip-slots]

# Dependency graph
requires: []
provides:
  - GA::EquipSlot constants for all 12 player equip points plus 8 cosmetic identifiers
  - GA::SlotValueId() / GA::EquipPointFromSlotValueId() bidirectional mapping functions
  - GA::DeviceId namespace with 28 device ID constants nested by class
  - GA::DeviceId::Lookup() runtime lookup via sqlite3 parameterized query
  - GA::Quality constants for None/Common/Uncommon/Rare/Epic quality tiers
affects: [02-inventory-equip-api, 03-refactor-existing-code]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - constexpr int in nested namespaces (no enum class) for zero-overhead typed constants
    - sqlite3_prepare_v2 + sqlite3_bind_text for parameterized DB lookups

key-files:
  created:
    - src/GameServer/Constants/EquipSlot.hpp
    - src/GameServer/Constants/Quality.hpp
    - src/GameServer/Constants/DeviceIds.hpp
    - src/GameServer/Constants/DeviceIds.cpp
  modified: []

key-decisions:
  - "constexpr int everywhere, no enum class — avoids casts at all call sites"
  - "Cosmetic identifiers use negative values to avoid collisions with real equip points (1-24)"
  - "SlotValueId() and EquipPointFromSlotValueId() are constexpr switch functions mirroring FUN_109a1320 exactly"

patterns-established:
  - "GA:: top-level namespace with nested sub-namespaces per domain"
  - "Device IDs organized by class namespace: GA::DeviceId::Assault::ImpactHammer"
  - "Parameterized sqlite3 query pattern via sqlite3_prepare_v2 + sqlite3_bind_text"

requirements-completed: [ENUM-01, ENUM-02, ENUM-03, ENUM-04]

# Metrics
duration: 5min
completed: 2026-03-21
---

# Phase 1 Plan 01: Enums and Constants Summary

**constexpr int constants for 28 device IDs (nested by class), 12 equip slots with bidirectional slot-value-ID mapping, and quality tiers — with sqlite3-backed runtime device lookup**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-03-21T00:40:00Z
- **Completed:** 2026-03-21T00:45:06Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- GA::EquipSlot: 12 player equip points + 8 cosmetic identifiers (negative values, no collision risk)
- GA::SlotValueId() and GA::EquipPointFromSlotValueId(): full bidirectional mapping for all 24 slots, constexpr switch mirroring FUN_109a1320
- GA::DeviceId: 28 device constants across Assault/Medic/Recon/Robotic/Jetpack namespaces, plus RestDevice
- GA::DeviceId::Lookup(): queries asm_data_set_items via parameterized sqlite3, returns 0 with log warning on miss
- GA::Quality: None/Common/Uncommon/Rare/Epic with correct value IDs from TgObject.uc (1162-1165)

## Task Commits

Each task was committed atomically:

1. **Task 1: Create EquipSlot.hpp and Quality.hpp** - `3827396` (feat)
2. **Task 2: Create DeviceIds.hpp and DeviceIds.cpp** - `77925cc` (feat)

## Files Created/Modified
- `src/GameServer/Constants/EquipSlot.hpp` - Equip point constants, cosmetic identifiers, bidirectional mapping functions
- `src/GameServer/Constants/Quality.hpp` - Quality tier constants
- `src/GameServer/Constants/DeviceIds.hpp` - 28 device ID constants nested by class + Lookup() declaration
- `src/GameServer/Constants/DeviceIds.cpp` - DeviceId::Lookup() sqlite3 implementation

## Decisions Made
- Used `constexpr int` throughout, no `enum class` — avoids casts at every call site (per plan spec)
- Cosmetic identifiers use negative values (-1 through -8) to stay in same namespace without colliding with equip points 1-24
- SlotValueId()/EquipPointFromSlotValueId() are constexpr switch functions so they can be used in other constexpr contexts and compile away entirely when called with constants

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- Plan's automated verify check used `grep "Assault::"` but the header uses `namespace Assault {` (correct nested namespace declaration). All done criteria verified manually — 28 device constants present, Lookup declared/implemented, everything in GA namespace.

## Next Phase Readiness
- All 4 constant files are ready for inclusion by Phase 2 (Inventory::Equip API)
- GA::EquipSlot::Jetpack (5), GA::SlotValueId(5) -> 201, GA::EquipPointFromSlotValueId(201) -> 5 all correct
- GA::DeviceId::Medic::Agonizer resolves to 2991, GA::Quality::Epic resolves to 1162

---
*Phase: 01-enums-and-constants*
*Completed: 2026-03-21*
