---
gsd_state_version: 1.0
milestone: v0.0.6
milestone_name: Class-Aware Spawning
status: in-progress
last_updated: "2026-03-21"
progress:
  total_phases: 2
  completed_phases: 0
  total_plans: 1
  completed_plans: 1
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-21)

**Core value:** Equip any device on any pawn with a single function call
**Current focus:** Phase 4 -- Class Identity

## Current Position

Phase: 4 of 5 (Class Identity)
Plan: 1 of 1 (04-01 complete)
Status: Phase 4 plan 1 complete -- advancing to Phase 5
Last activity: 2026-03-21 -- 04-01 executed (ClassConfig + class-aware SpawnPlayerCharacter)

Progress: [█░░░░░░░░░] 10%

## Accumulated Context

### Decisions

- v0.0.5: constexpr int for all constants (avoids casts, zero overhead)
- v0.0.5: Batch equip pattern (N x Equip + 1 x Finalize) avoids redundant UpdateClientDevices
- v0.0.5: Explicit per-device Equip calls in SpawnPlayerCharacter (self-documenting loadout)
- v0.0.6 (04-01): ClassConfig includes Phase 5 device ID fields so GetClassConfig is the single loadout source
- v0.0.6 (04-01): nPendingBotId = profileId (player classes use profile_id as bot_id in DB)
- v0.0.6 (04-01): HP/power read-back from InitializeDefaultProps (HealthMax, r_fMaxPowerPool) not hardcoded

### Key Facts for v0.0.6

- Profile IDs: Assault=680, Medic=567, Recon=681, Robotic=679
- Skill group set IDs: Assault=19, Medic=11, Recon=17, Robotic=18
- Jetpack device IDs: Assault=7031, Medic=7032, Recon=7033, Robotic=7034
- selected_profile_id readable via GClientConnectionsData[connIdx].pPlayerInfo->selected_profile_id
- All identity fields (CLID) and all equipment (EQUP) go into SpawnPlayerCharacter.cpp

### Blockers/Concerns

None yet.

## History

- 2026-03-20: Project initialized with 3 phases, 13 requirements
- 2026-03-21: v0.0.5 shipped -- 3 phases, 4 plans, 858 lines added
- 2026-03-21: v0.0.6 roadmap created -- 2 phases (4-5), 12 requirements
- 2026-03-21: 04-01 executed -- ClassConfig struct, GetClassConfig, class-aware SpawnPlayerCharacter

---
*Last updated: 2026-03-21 after 04-01 execution*
