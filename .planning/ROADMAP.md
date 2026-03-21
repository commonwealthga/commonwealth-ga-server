# Roadmap: Inventory API

**Created:** 2026-03-20
**Milestone:** v1 — Clean Inventory API

## Phases

### Phase 1: Enums & Constants
**Goal:** Define all slot, device, and quality constants so the API has a vocabulary to work with.
**Requirements:** ENUM-01, ENUM-02, ENUM-03, ENUM-04
**Plans:** 1/1 plans complete

Plans:
- [ ] 01-01-PLAN.md — EquipSlot + Quality + DeviceId constants with bidirectional mapping and runtime lookup

**Success Criteria:**
1. EquipSlot enum covers all 11 game slots with correct slot value IDs
2. DeviceId constants match actual device IDs from the game data
3. Quality constants map to correct quality value IDs
4. Equip point mapping produces correct results for all slots

### Phase 2: Inventory::Equip API
**Goal:** Single function call that handles the full device equip flow end-to-end.
**Requirements:** API-01, API-02, API-03, API-04, API-05, API-06

**Success Criteria:**
1. Inventory::Equip(Pawn, DeviceId::Agonizer, EquipSlot::Ranged, Quality::Epic) works
2. Device appears on client with correct visuals
3. No manual steps needed — function handles inventory object, instance ID, item count, replication
4. Quality parameter is optional

### Phase 3: Refactor Existing Code
**Goal:** Replace manual equip code in SpawnPlayerCharacter and GiveJetpack with the new API.
**Requirements:** REF-01, REF-02, REF-03

**Success Criteria:**
1. SpawnPlayerCharacter uses Inventory::Equip for all devices
2. GiveJetpack uses Inventory::Equip
3. Player spawns with correct loadout (no visual or functional regression)

## Phase Summary

| # | Phase | Requirements | Status |
|---|-------|-------------|--------|
| 1 | 1/1 | Complete   | 2026-03-21 |
| 2 | Inventory::Equip API | API-01..06 | Pending |
| 3 | Refactor Existing Code | REF-01..03 | Pending |

---
*Roadmap created: 2026-03-20*
