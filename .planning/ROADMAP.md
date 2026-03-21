# Roadmap: Commonwealth GA Server

## Milestones

- ✅ **v0.0.5 Clean Inventory API** -- Phases 1-3 (shipped 2026-03-21)
- 🚧 **v0.0.6 Class-Aware Spawning** -- Phases 4-5 (in progress)

## Phases

<details>
<summary>✅ v0.0.5 Clean Inventory API (Phases 1-3) -- SHIPPED 2026-03-21</summary>

- [x] Phase 1: Enums & Constants (1/1 plans) -- completed 2026-03-21
- [x] Phase 2: Inventory::Equip API (1/1 plans) -- completed 2026-03-21
- [x] Phase 3: Refactor Existing Code (2/2 plans) -- completed 2026-03-21

</details>

### v0.0.6 Class-Aware Spawning (In Progress)

**Milestone Goal:** Players spawn with the correct class identity and equipment based on their selected profile.

### Phase 4: Class Identity
**Goal**: SpawnPlayerCharacter reads the selected class and sets all identity fields on Pawn and PRI
**Depends on**: Phase 3
**Requirements**: CLID-01, CLID-02, CLID-03, CLID-04, CLID-05
**Success Criteria** (what must be TRUE):
  1. Spawned player's Pawn r_nProfileId matches the class they selected (Assault=680, Medic=567, Recon=681, Robotic=679)
  2. PRI r_nProfileId matches Pawn r_nProfileId for the spawned player
  3. Spawned player's Pawn r_nSkillGroupSetId is set to the correct value for their class
  4. nPendingBotId is set to the class bot ID before Pawn spawn so InitializeDefaultProps loads the correct class defaults
**Plans:** 1 plan
Plans:
- [x] 04-01-PLAN.md — ClassConfig struct + class-aware SpawnPlayerCharacter + TcpSession consolidation

### Phase 5: Class Equipment
**Goal**: Spawned player receives the full correct device loadout for their class across all equip slots
**Depends on**: Phase 4
**Requirements**: EQUP-01, EQUP-02, EQUP-03, EQUP-04, EQUP-05, EQUP-06, EQUP-07
**Success Criteria** (what must be TRUE):
  1. Spawned player has the correct melee and ranged weapons equipped for their class
  2. Spawned player has the correct specialty device and jetpack equipped for their class
  3. Spawned player has all 3 offhand devices equipped in Offhand1/Offhand2/Offhand3 slots for their class
  4. Spawned player has the correct morale boost device, RestDevice, and base attributes device equipped
**Plans**: TBD

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Enums & Constants | v0.0.5 | 1/1 | Complete | 2026-03-21 |
| 2. Inventory::Equip API | v0.0.5 | 1/1 | Complete | 2026-03-21 |
| 3. Refactor Existing Code | v0.0.5 | 2/2 | Complete | 2026-03-21 |
| 4. Class Identity | v0.0.6 | 1/1 | Complete | 2026-03-21 |
| 5. Class Equipment | v0.0.6 | 0/1 | Not started | - |

---
*Roadmap created: 2026-03-20*
*Last updated: 2026-03-21 after Phase 4 execution (04-01 complete)*
