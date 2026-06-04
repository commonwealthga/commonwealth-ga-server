# Effect / buff / property system

High-level orientation for the effect/buff/property subsystem. There is an active rebuild in progress — verify any specific class / address claim against current source before trusting.

## The four collaborating systems

1. **Effects** (`UTgEffect` + subclasses) — instantaneous or interval-tick state mutations: damage, heal, set property, apply buff. One UC class per effect type.
2. **Effect Groups** (`UTgEffectGroup`) — bundles of effects that share a lifetime, source device, and target. The unit of apply / remove / displacement.
3. **Buffs** (`m_EffectBuffInfo` on pawn) — sparse aggregator that scales other effects' outputs by per-property modifiers. NOT a separate effect type — it's a side-effect layer that effects read from when computing their actual value.
4. **Properties** (`s_Properties` on pawn, `r_xxx` mirror fields) — the actual gameplay state (HEALTH, GROUND_SPEED, PHYSICAL_PROTECTION, etc.). 27+ standard props. Property writes go through `SetProperty(propId, value)` → `ApplyProperty(descriptor)` which fans out to engine fields, replicates, and clamps current-to-max.

## Class → DB mapping

`asm_data_set_effects.class_res_id` selects the UC class instantiated for that effect:

- `80`  → `TgEffect` (base, generic property set)
- `89`  → `TgEffectVisibility`
- `157` → `TgEffectBuff`
- `181` → `TgEffectDamage`
- `244` → `TgEffectSensor`
- `692` → `TgEffectHeal`

When building an effect group from DB, dispatch on `class_res_id` to choose the constructor.

## Template / clone model

Effect groups have two lifetime stages:

1. **Template** — built once per (device, fire mode) pair, cached in `s_EffectGroupList` on the firing device. Holds the DB-loaded defaults: `m_fBase`, `m_fMinimum`, `m_fMaximum`, `m_fLifeTime`, `m_fInterval`, etc.
2. **Clone** — `CloneEffectGroup` allocates a fresh `UTgEffectGroup` + fresh `UTgEffect` children per impact. Lives in `s_AppliedEffectGroups` on the target pawn. Lifetime managed by `LifeDone` timer.

**Cloning must allocate fresh `UTgEffect` children, not share template pointers.** Shared `m_fCurrent` causes buff/protection to stick after leaving radius, and stacks weirdly from multiple sources. Fixed 2026-05-08; don't regress it.

## m_bIsManaged and m_bRemovable

Two flags on effect groups that control HUD-slot and reverse behavior:

- `m_bIsManaged=1` → has a HUD slot, gets a `s_ManagedEffectListIndex`. Used for player-visible buffs (the icons on the side of the screen).
- `m_bIsManaged=0` → no HUD slot. Used for instantaneous effects (damage, heal) and lifetime=0 modifiers.
- `m_bRemovable=1` → eligible for `RemoveEffectGroup` and per-effect `eventRemove` (i.e. its `Apply` is reversible).
- `m_bRemovable=0` → permanent until the effect group is fully destroyed.

`s_ManagedEffectListIndex` must default to `-1` on cloned groups. Phantom-clone (skipped-Apply) groups inherit slot 0 by default, and on later cleanup zero slot 0 of an UNRELATED group's HUD icon. Fixed 2026-05-07.

## Displacement & revert

When a new effect group of the same category lands on a pawn, the system displaces (or refreshes) the existing one. Two displacement modes:

- **Newest wins** — new value replaces, old reverses.
- **Strongest wins** — new value replaces only if greater, otherwise discard.

Per-category. UC `TgEffect.uc` has the displacement table.

`RemoveAllEffectGroups` and `RemoveEffectGroup` must:

1. Run per-effect `eventRemove` on the displaced group (reverse aggregator).
2. Zero the displaced group's `s_ManagedEffectListIndex` ONLY when its lifetime was > 0 (lifetime=0 + interval=0 groups are aura ticks, slot=0 mutation would zero an unrelated group's slot).
3. Mark `EffectDisplacementMarker` for the displaced (manager, egId) pair so the next `SetEffectRep` queue-push can consume the marker and decide whether to push.

The displacement marker is currently time-based (~2ms fence) which is fragile against GC pauses. Two follow-up options live in conversation: widen threshold to 50ms, or replace with a thread-local apply-depth structural signal.

## AOI: reverse on death

`RemoveAllEffects` (death cleanup) classifies effects per `aoi` flag:

- `aoi=1` → tick gift (regen ticker, station-pad effects, power-pool grants). SKIP reversal — these are intentionally one-shot effects.
- `aoi=0` → modifier. REVERSE — these change pawn state and must be undone on death.

Classification is **per effect**, not per group. The old coarse gate left REST device penalties permanently applied after revive. Fixed 2026-05-20.

Adjacent: `RemoveAllEffects` must ALSO fire `ProcessReactiveSkillBasedEffectGroup(cat, true)` for every category whose last group it strips (mirrors `RemoveEffectGroup`'s last-of-cat check) AND zero `r_nShieldHealthMax` / `r_nShieldHealthRemaining` when any health>0 group was removed. Without the first, Aegis Armament's +25 stat-mod leaks across death (permanent +25% phys resist exploit). Without the second, the shield bar stays visible on the corpse/respawn.

## Skill clones — RCST on revive

Skills are baked into `s_AppliedEffectGroups` as `lifetime=0 m_bIsManaged=1` clones at character load (`RCST` = ReapplyCharacterSkillTree, approximate name). Death's `RemoveAllEffects` strips them. So `ReapplyLoadoutEffects` (Dying.EndState, Arena restart, anywhere that re-runs post-death setup) must ALSO call RCST or skill buffs (prop 256 accuracy, 214/232/etc) stay lost until next respec.

## Buff system — `m_EffectBuffInfo`

The buff layer is a sparse aggregator on each pawn. Effects whose `class_res_id=157` (`TgEffectBuff`) modify it on apply, reverse on remove.

Key concepts:

- **`GetBuffIndex(buffHeader, startIdx, bAddIfNotExists)`** — locates / inserts a buff entry. The binary uses **exact match** when `bAddIfNotExists=1` (apply path) but **wildcard match** when `bAddIfNotExists=0` (reverse path). Stored=0 in any BuffHeader field swallows non-zero queries during reverse → reverse debits the wrong slot → drift. Fix is an exact-match scan on the reverse path. Burned this once on medstations heal cycle (-10%/cycle drift).
- **`startIdx` must be ≥ 0.** Negative seeds cause UB array reads.
- **Source classification** — `srcType=ITEM` (1) vs `srcType=OTHER` (4) selects which aggregator layer the buff lands in. Rolled-mod prop 385 (Output Mod) must register as `OTHER` (4), not `ITEM` (1). `ITEM` collapses prop 330 (+Heal) and prop 385 (Output Mod) into a single bucket; `OTHER` routes through the GP layer for multiplicative composition.
- **Three-piece replication chain** — for buffs to actually take effect on a remote client: (1) bypass SDK `GetBuffIndex` wrapper (drops `nIndex` input → duplicate entries), (2) call `eventClientSendBuff` after server apply (else client never sees buff, no crosshair tighten), (3) intercept stripped `CheckEffectBuffModifier` via `ProcessEvent` (else damage buff prop 65 silently no-ops). All three needed.

## CheckEffectBuffModifier — query rules

Query the buff aggregator with `BUFF_PAWN/effect.propId`, NEVER `BUFF_OTHER/65`. `BUFF_OTHER/65` returns identity `{65}` and silently drops Output Mod (prop 385) + per-damage-type modifiers; halves weapon damage.

For Heal effects (`TgEffectHeal` class), normalize the query propId to `51` regardless of the effect's actual propId. Props 211 (Missing Health) and 260 (Repair) have no expansion in `ConvertPropToPropList` → no Output Mod / heal-mod scaling. Rewriting `queryPropId=51` in `CheckEffectBuffModifier` when the class is `TgEffectHeal` routes through the canonical `{330, 385, +376}` expansion.

## CloneEffectGroup pre-scaling

UC `CheckEffectBuffModifier` and `TakeDamage` are both UC-to-UC dispatched (inline VM bypass of `ProcessEvent` via `EX_FinalFunction`). You CANNOT intercept them via `ProcessEvent` hooks.

The working pattern: pre-scale the cloned effect's `m_fBase` / `m_fMinimum` / `m_fMaximum` in `CloneEffectGroup` using the buffed prop-65 value. UC then reads the scaled fields directly.

## ApplyDeployableSetup clears device-mode

Deployable Apply re-asserts the device template, which clears `s_SpawnerDeviceMode`. Station OUTPUT buffs (e.g. medical station Output Mod +70%) live on the spawner's device-mode — clearing it loses the modifier. The medical station healed 184 instead of 491 because of this. Re-assert `s_SpawnerDeviceMode` after `ApplyDeployableSetup`. Affects all stations.

## ApplyBuffEffectFromHook — DO NOT pre-zero m_fCurrent

`ProcessEvent` intercepts fire BEFORE UC bytecode runs. Pre-zeroing `m_fCurrent` makes UC's `ApplyToProperty` compute `m_fRaw - 0 = m_fRaw` (no-op). Property modifier (e.g. jetpack +115 AirSpeed) leaks per fire pulse, compounds over session.

UC sets `m_fCurrent=0` itself after `ApplyToProperty`. Trust it.

## DeployableOriginRegistry

UC `TgEffect.InitInstance` unconditionally clears source-device fields before checking Impact. For deployable heals with no `DeviceModeReference`, the fields stay 0 → device-scoped buffs reject the apply.

Workaround: `DeployableOriginRegistry::RegisterClone(clone, origin)` / `LookupClone(clone)` preserves the deploy origin across `InitInstance`, and `CheckEffectBuffModifier` looks it up before falling back to clone fields.

## Pet buffs

- **Don't bridge prop 350 PetDamage in `ApplyPlayerModsToPet`.** Bridging player.350 → pet.65 double-applies with fire-time `CheckOwnerPetBuff(350)`. Removed once owner-side native was reimplemented. Other bridges (range/radius/accuracy/HP/lifespan) stay.
- **`CheckOwnerPetBuff` must query with the pet's spawner devInst, not 0.** Hard-coded `nReqDeviceInstId=0` skips every device-scoped pet-damage mod + Output Mod. Use `petPawn->s_nSpawnerDeviceInstId` (offset 0x1070).

## SetEffectRep queue-push gate

`SetEffectRep` has two replication paths:

- **Branch A** — queue push (one-shot per-tick FX, displaces existing).
- **Branch B** — refresh-in-place (sustains a per-tick FX for the lifetime of the group).

Classification: `vtable[0x39c] = GetEffectGroup @ 0x10a70d10` linearly searches `s_AppliedEffectGroups` for matching egId. UC always adds the clone before calling `SetEffectRep`, so Branch A (queue push) is unreachable in the normal flow. Per-tick FX must come from Branch B refresh-in-place.

Queue push fires only when **(suppressManaged) OR (Branch B succeeded AND (sameEgIdOthers≥1 OR wasJustDisplaced))**. Removes scope/repair-arm double FX while preserving per-tick on own pawn for Strongest/Newest displacements.

## Lifetime=0 hit-family suppresses HUD slot

`SetEffectRep` for type=264 / type=505 with `fTime=0` does NOT allocate a HUD slot (retail design puts icons on lifetime>0 companion EGs with `app=836 Refresh`, per Pain Gun pattern). Replaces the old `Target!=Instigator suppressManaged` check.

Other types (263 Fire / 266 Aim / 261 Equip / 283 Equip Mode) keep slots — they have explicit cleanup paths.

## TgDeployable.r_EffectManager must be spawned manually

`TgDeployable.uc` has no `defaultsubobject` / `PostBeginPlay` spawn for `r_EffectManager` (TgPawn does). Without manual `Spawn()` in our code, `TgDeployable.ProcessEffect` Accessed-None's at `r_EffectManager.ProcessEffect(...)` and damage/repair silently no-op. Bootstrap in `SpawnDeployableActor`.

## Verifying any specific claim

This doc is a map, not a contract. Before relying on a specific egId / propId / address / vtable offset, verify against current source. The rewrite is in progress; some addresses and fields are moving targets.
