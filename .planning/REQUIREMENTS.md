# Requirements: Commonwealth GA Server -- Class-Aware Spawning

**Defined:** 2026-03-21
**Core Value:** Equip any device on any pawn with a single function call

## v0.0.6 Requirements

Requirements for class-aware player spawning. Each maps to roadmap phases.

### Class Identity

- [x] **CLID-01**: SpawnPlayerCharacter reads selected_profile_id from GClientConnectionsData for the spawning player
- [x] **CLID-02**: Pawn r_nProfileId is set to the selected class profile ID (Assault=680, Medic=567, Recon=681, Robotic=679)
- [x] **CLID-03**: PRI r_nProfileId is set to match Pawn's profile ID
- [x] **CLID-04**: Pawn r_nSkillGroupSetId is set per class (Assault=19, Medic=11, Recon=17, Robotic=18)
- [x] **CLID-05**: nPendingBotId is set to the class's bot_id before Pawn spawn

### Equipment

- [ ] **EQUP-01**: Each class equips its correct melee weapon (ImpactHammer/LifeStealer/DualDaggers/MaceAndShield)
- [ ] **EQUP-02**: Each class equips its correct ranged weapon (RhinoSMG/Agonizer/Ballista/ColonyEnergyRifle)
- [ ] **EQUP-03**: Each class equips its correct specialty device (InfernoXCannon/AdrenalineGun/SpringStealth/FocusedRepairArm)
- [ ] **EQUP-04**: Each class equips its correct jetpack (Assault=7031, Medic=7032, Recon=7033, Robotic=7034)
- [ ] **EQUP-05**: Each class equips all 3 offhand devices in slots Offhand1/Offhand2/Offhand3
- [ ] **EQUP-06**: Each class equips its correct morale boost device
- [ ] **EQUP-07**: Each class equips RestDevice (864) and the base attributes device at equip point 14

## Future Requirements

### Equipment Management

- **EQMG-01**: Player can swap individual devices in loadout
- **EQMG-02**: Loadout persists across respawns via database

## Out of Scope

| Feature | Reason |
|---------|--------|
| Per-class body mesh | DB shows all classes share body_asm_id=173; differentiation is via profileId + equipment |
| Persistent loadout customization | Future work -- currently spawning with fixed default loadout per class |
| Remove/swap API | Not needed for spawn-time equipping |
| Web-based loadout editor | Long-term vision, not current scope |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| CLID-01 | Phase 4 | Complete (04-01) |
| CLID-02 | Phase 4 | Complete (04-01) |
| CLID-03 | Phase 4 | Complete (04-01) |
| CLID-04 | Phase 4 | Complete (04-01) |
| CLID-05 | Phase 4 | Complete (04-01) |
| EQUP-01 | Phase 5 | Pending |
| EQUP-02 | Phase 5 | Pending |
| EQUP-03 | Phase 5 | Pending |
| EQUP-04 | Phase 5 | Pending |
| EQUP-05 | Phase 5 | Pending |
| EQUP-06 | Phase 5 | Pending |
| EQUP-07 | Phase 5 | Pending |

**Coverage:**
- v0.0.6 requirements: 12 total
- Mapped to phases: 12
- Unmapped: 0

---
*Requirements defined: 2026-03-21*
*Last updated: 2026-03-21 after 04-01 execution (CLID-01 through CLID-05 complete)*
