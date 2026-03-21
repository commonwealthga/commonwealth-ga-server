---
phase: 03-refactor-existing-code
plan: 01
subsystem: inventory
tags: [cpp, inventory, equip, device-ids, refactor]

requires:
  - phase: 02-inventory-equip-api
    provides: Inventory::Equip, Inventory::Finalize, EquippedEntry, GetEquipped, ClearTracking

provides:
  - Inventory::NextId() as single source of truth for inventory IDs
  - effectGroupId field in EquippedEntry for inventory marshal usage
  - GetEquippedByPawnId(int pawnId) for marshal lookups without ATgPawn pointer
  - SpawnPlayerCharacter equips all 7 medic devices via explicit Inventory::Equip calls
  - GiveDevicesFromBotConfig unified to use Inventory::NextId()
  - Inventory.cpp and DeviceIds.cpp added to Makefile build

affects: [04-inventory-marshal, send_inventory_response, TcpSession]

tech-stack:
  added: []
  patterns:
    - "Inventory::NextId() is the only inventory ID counter in the codebase — never use local counters"
    - "EquippedEntry.effectGroupId populated at equip time via GetEffectGroupId(deviceId) hardcoded switch"
    - "s_equippedByPawnId map mirrors s_equipped keyed by pawnId for marshal lookups"

key-files:
  created: []
  modified:
    - src/GameServer/Inventory/Inventory.hpp
    - src/GameServer/Inventory/Inventory.cpp
    - src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp
    - src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp
    - src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.cpp
    - Makefile

key-decisions:
  - "GetEffectGroupId uses hardcoded switch keyed by deviceId; comment notes to replace with DB lookup when device->effect_group relation exists"
  - "ClearTracking iterates s_equippedByPawnId to find and remove any entry pointing to s_equipped[Pawn] before erasing from s_equipped"
  - "REST equip at slot 14 and HealingBoost at slot 10 are hardcoded from bot_id=567 DB config per existing send_inventory_response"
  - "specialty device 2906 used as raw integer with comment since it is not in DeviceId::Medic namespace"

patterns-established:
  - "Single ID counter: Inventory::NextId() is the canonical source — removed nInventoryIdCounter from SpawnBotById"
  - "Self-documenting loadouts: explicit Inventory::Equip calls per device, not opaque GiveDevicesFromBotConfig"

requirements-completed: [REF-01, REF-02]

duration: 12min
completed: 2026-03-21
---

# Phase 03 Plan 01: Refactor SpawnPlayerCharacter and Unify Inventory ID Counter

**Inventory::Equip replaces GiveDevicesFromBotConfig for player loadout; NextId() unifies all inventory ID generation; effectGroupId and pawnId tracking added for downstream marshal rewrite**

## Performance

- **Duration:** 12 min
- **Started:** 2026-03-21T07:51:00Z
- **Completed:** 2026-03-21T08:03:34Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- SpawnPlayerCharacter now calls Inventory::Equip for each of the 7 medic loadout devices (LifeStealer, Agonizer, specialty 2906, Medic CJP jetpack, HealingGrenade, HealingBoost, REST) plus Inventory::Finalize — loadout is self-documenting
- GiveDevicesFromBotConfig uses Inventory::NextId() instead of local nInventoryIdCounter — single source of truth for all inventory IDs in the codebase
- EquippedEntry gains effectGroupId field populated at equip time; GetEquippedByPawnId enables marshal lookups by pawnId without needing the ATgPawn pointer

## Task Commits

Each task was committed atomically:

1. **Task 1: Extend Inventory API with NextId, effectGroupId, and pawnId tracking** - `730906c` (feat)
2. **Task 2: Refactor SpawnPlayerCharacter and unify ID counter in GiveDevicesFromBotConfig** - `17a898b` (feat)

## Files Created/Modified

- `src/GameServer/Inventory/Inventory.hpp` - Added effectGroupId to EquippedEntry; added NextId(), GetEquippedByPawnId(), s_equippedByPawnId, GetEffectGroupId() declarations
- `src/GameServer/Inventory/Inventory.cpp` - Implemented NextId(), GetEffectGroupId() switch, populated effectGroupId+s_equippedByPawnId in Equip(), added GetEquippedByPawnId(), updated ClearTracking()
- `src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp` - Added Inventory/EquipSlot/DeviceIds includes; replaced GiveDevicesFromBotConfig call with 7 Inventory::Equip + Finalize
- `src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp` - Added Inventory.hpp include; removed nInventoryIdCounter field
- `src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.cpp` - Removed nInventoryIdCounter static init; replaced usages with Inventory::NextId()
- `Makefile` - Added Inventory.cpp and DeviceIds.cpp to SOURCE_FILES

## Decisions Made

- GetEffectGroupId uses a hardcoded switch; a comment marks the intended future migration to DB lookup once a device->effect_group relation is available
- ClearTracking safely removes from s_equippedByPawnId by iterating to find the matching pointer before erasing from s_equipped
- Used raw integer 2906 for the specialty device (not in DeviceId::Medic namespace) with an explanatory comment
- REST at equip point 14 and HealingBoost at equip point 10 match the bot_id=567 DB config as observed in the existing send_inventory_response

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- GetEquippedByPawnId and effectGroupId are available for the send_inventory_response rewrite in Plan 02
- Inventory::NextId() is the canonical ID source; no other counter exists
- Makefile correctly includes Inventory.cpp and DeviceIds.cpp for compilation

## Self-Check: PASSED

- FOUND: src/GameServer/Inventory/Inventory.hpp
- FOUND: src/GameServer/Inventory/Inventory.cpp
- FOUND: src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.cpp
- FOUND: src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.cpp
- FOUND: .planning/phases/03-refactor-existing-code/03-01-SUMMARY.md
- FOUND: commit 730906c (Task 1)
- FOUND: commit 17a898b (Task 2)

---
*Phase: 03-refactor-existing-code*
*Completed: 2026-03-21*
