# Roadmap: Inventory API

**Created:** 2026-03-20
**Milestone:** v1 — Clean Inventory API

## Phases

### Phase 1: Enums & Constants
**Goal:** Define all slot, device, and quality constants so the API has a vocabulary to work with.
**Requirements:** ENUM-01, ENUM-02, ENUM-03, ENUM-04
**Plans:** 1/1 plans complete

Plans:
- [x] 01-01-PLAN.md — EquipSlot + Quality + DeviceId constants with bidirectional mapping and runtime lookup

**Success Criteria:**
1. EquipSlot enum covers all 11 game slots with correct slot value IDs
2. DeviceId constants match actual device IDs from the game data
3. Quality constants map to correct quality value IDs
4. Equip point mapping produces correct results for all slots

### Phase 2: Inventory::Equip API
**Goal:** Single function call that handles the full device equip flow end-to-end.
**Requirements:** API-01, API-02, API-03, API-04, API-05, API-06
**Plans:** 1 plan

Plans:
- [x] 02-01-PLAN.md — Inventory class with Equip/Finalize/GetEquipped API and per-pawn tracking

**Success Criteria:**
1. Inventory::Equip(Pawn, DeviceId::Agonizer, EquipSlot::Ranged, Quality::Epic) works
2. Device appears on client with correct visuals
3. No manual steps needed — function handles inventory object, instance ID, item count, replication
4. Quality parameter is optional

### Phase 3: Refactor Existing Code
**Goal:** Replace GiveDevicesFromBotConfig call in SpawnPlayerCharacter with Inventory::Equip API, rewrite send_inventory_response to use Inventory tracking, and unify inventory ID counter.
**Requirements:** REF-01, REF-02, REF-03
**Plans:** 2 plans

Plans:
- [x] 03-01-PLAN.md — Extend Inventory API (NextId, effectGroupId, pawnId tracking) + refactor SpawnPlayerCharacter + unify ID counter
- [ ] 03-02-PLAN.md — Rewrite send_inventory_response to use Inventory::GetEquippedByPawnId loop + verify compilation

**Success Criteria:**
1. SpawnPlayerCharacter uses Inventory::Equip for all devices
2. GiveJetpack/GiveAgonizer kept as-is (dead code, harmless)
3. Player spawns with correct loadout (no visual or functional regression)
4. send_inventory_response is data-driven from Inventory tracking
5. Single inventory ID source: Inventory::NextId()

## Phase Summary

| # | Phase | Requirements | Status |
|---|-------|-------------|--------|
| 1 | Enums & Constants | ENUM-01..04 | Complete (2026-03-21) |
| 2 | Inventory::Equip API | API-01..06 | Complete (2026-03-21) |
| 3 | Refactor Existing Code | REF-01..03 | In Progress (1/2 plans) |

---
*Roadmap created: 2026-03-20*
