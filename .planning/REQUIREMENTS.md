# Requirements: Inventory API

**Defined:** 2026-03-20
**Core Value:** Equip any device on any pawn with a single function call

## v1 Requirements

### Enums & Constants

- [x] **ENUM-01**: EquipSlot enum with all 11 game slots (Melee, Ranged, Specialty, Jetpack, Offhand1-3, Morale, Rest, Beacon, Consumable)
- [x] **ENUM-02**: Named DeviceId constants for known devices (Agonizer, MedicJetpack, etc.) populated from DB or UnrealScript
- [x] **ENUM-03**: Quality enum/constants mapping quality names to value IDs
- [x] **ENUM-04**: Slot-to-equip-point mapping table (slot value ID → equip point, wrapping FUN_109a1320)

### API

- [ ] **API-01**: Inventory::Equip(Pawn, deviceId, slot, quality) function that handles the full equip flow
- [ ] **API-02**: Equip function creates inventory object via CreateEquipDevice
- [ ] **API-03**: Equip function assigns unique r_nDeviceInstanceId (non-zero)
- [ ] **API-04**: Equip function syncs r_ItemCount with m_InventoryMap
- [ ] **API-05**: Equip function triggers UpdateClientDevices for client replication
- [ ] **API-06**: Quality parameter is optional, defaults to no quality (0)

### Refactor

- [x] **REF-01**: Refactor SpawnPlayerCharacter to use Inventory::Equip instead of manual equip code
- [x] **REF-02**: Refactor GiveJetpack to use Inventory::Equip
- [ ] **REF-03**: Verify client sees equipped devices after refactor (no regression)

## v2 Requirements

### Extended API

- **EXT-01**: Inventory::Remove(Pawn, slot) to unequip a device
- **EXT-02**: Inventory::GetEquipped(Pawn, slot) to query current device
- **EXT-03**: String-based overloads for TCP/web integration

## Out of Scope

| Feature | Reason |
|---------|--------|
| Persistent inventory | Future work — requires vendor/shop system |
| Web loadout editor | Long-term vision, not current milestone |
| Remove/swap API | Not needed for hardcoded loadout use case |
| String-based lookups | Enum-only for now |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| ENUM-01 | Phase 1 | Complete |
| ENUM-02 | Phase 1 | Complete |
| ENUM-03 | Phase 1 | Complete |
| ENUM-04 | Phase 1 | Complete |
| API-01 | Phase 2 | Pending |
| API-02 | Phase 2 | Pending |
| API-03 | Phase 2 | Pending |
| API-04 | Phase 2 | Pending |
| API-05 | Phase 2 | Pending |
| API-06 | Phase 2 | Pending |
| REF-01 | Phase 3 | Complete |
| REF-02 | Phase 3 | Complete |
| REF-03 | Phase 3 | Pending |

**Coverage:**
- v1 requirements: 13 total
- Mapped to phases: 13
- Unmapped: 0 ✓

---
*Requirements defined: 2026-03-20*
