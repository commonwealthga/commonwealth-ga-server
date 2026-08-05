# Effective vs. ineffective device use — how the game decides, and where to observe it

Handoff for the device-usage metrics work. Covers four things the current per-instance device
stats do not record: **cleanses**, **Triage Wave's conditional half**, **Group Heal Savior**, and
**boost activations**.

Everything here is about the **server**, not the theorycrafting console. Ids are given in
parentheses after names only where you will need them to query.

**Verification status.** Effect-group anatomy, property ids and device/skill names are read
directly from `gaa.db` (not the repo's `server.db` — that one does not have the populated
`asm_data_set_*` tables). Code paths are read from `src/`. The `ApplyHit` dispatch order in §2 is
quoted from `docs/claude/theorycraft-console/issues/killer-instinct-diag.md`, **which lives on the
`theorycraft-console` branch, not on `master`** — check that branch out to read it. It took the
order from the UnrealScript source and verified the resulting behaviour in game on 2026-08-02; I
did not re-derive it. Three things I could **not** verify are flagged inline as ⚠ and collected in §8.

---

## 1. The two recording surfaces you already have

| | `DeviceStats` | `MatchStats` |
|---|---|---|
| Where | `src/GameServer/Stats/DeviceStats.{hpp,cpp}` | `src/GameServer/Stats/MatchStats.hpp` |
| Shape | counters on a per-device row | discrete event rows over IPC |
| Storage | `ATgRepInfo_Player::r_DeviceStats[9]`, replicated owner-only | `ga_match_events` via the control server |
| Reaches the client | yes, end-of-mission Device Stats tab | no |
| Cost per record | none (an `int +=`) | an IPC message |

**`FDeviceStatInfo::Stats` is 11 ints wide (`Stats[0xB]`). The server writes 0–6.**

```
0 device id   1 damage   2 healing   3 player kills   4 bot kills   5 DPM   6 HPM
7 8 9 10  ← unused, already replicating
```

So there are **four free replicated slots per device row** and you do not need to touch the struct
or the replication list to start counting. The catch is the other end: the client's row builder
(`UTgUIDeviceStats` vtable +0x130 → 0x11463870) walks the first 8 rows and prints `Stats[1..6]`
verbatim. Slots 7–10 will replicate and arrive intact but render nowhere without client-side work.

Rough guidance:

- **A counter that belongs next to damage/healing on the device tab** → `DeviceStats`, slots 7–10.
- **"this specific thing happened, with context"** → `MatchStats`. It carries `event_type`,
  `detail` and `flags`, so it can hold "which category was purged" or "target was at 18%" in a way
  a bare counter cannot.

Both are per-instance already, and `MatchStats` no-ops entirely unless `SetEnabled(true)` arrived
in `INSTANCE_HELLO_ACK` — home maps are off, which is almost certainly what you want.

---

## 2. The pipeline, once, because all four features hang off it

A device firing resolves synchronously, all the way to the property write:

```
TgDeviceFire.ApplyHit
  └─ SubmitHitEffects(instigator, impact, eSource, nType, nSituationalType)
       ├─ eSource 0 → TgDeviceFire.GetEffectGroup          (the device's own groups)
       └─ eSource 1 → TgEffectManager.GetSkillBasedEffectGroup  (the player's slotted skills)
            └─ SubmitEffect → TgEffectManager.ProcessEffect
                 └─ TgEffectGroup.ApplyEffects
                      └─ TgEffect.ApplyEffect → ApplyToProperty
```

`ApplyHit`'s dispatch order — this matters because it tells you *when* in a single impact each
thing is decided:

```
1404   SubmitHitEffects(.., 0, 272)             HIT_IN_AIR
1409   SubmitHitEffects(.., 0, 505, 1270)       device,  HP-below     ← Triage Wave
1410   SubmitHitEffects(.., 0, 505, 1271)       device,  HP-above
1411   SubmitHitEffects(.., 1, 505, 1270)       skill,   HP-below     ← Group Heal Savior
1412   SubmitHitEffects(.., 1, 505, 1271)       skill,   HP-above     ← Killer Instinct
1445   SubmitHitEffects(.., 0, 264)             device: base damage/heal, then device debuffs
1480   SubmitHitEffects(.., 0, 505, 509)        backstab
1491   SubmitHitEffects(.., 1, 505, melee)      situational melee
1495   SubmitHitEffects(.., 1, 264)             skill
```

Two consequences worth internalising:

1. **The HP-gated block runs BEFORE the damage/heal submit at 1445.** So when a Group Heal Savior
   or Triage Wave gate is evaluated, the target's health is still its *pre-heal* value. A target
   at 20% that your wave is about to heal to 60% is judged at 20% — which is the behaviour you
   want, but it means you cannot reconstruct the decision after the fact from post-impact health.
   **Record at decision time or not at all.**
2. Every other situational dispatch sits *after* the damage submit. Only the HP-gated block is
   ahead of it. That asymmetry was the root of the Killer Instinct self-shot bug and is documented
   as an authoring slip in the original data, not a design.

**Effect group types** (`effect_group_type_value_id`): 261 Equip · 263 Fire · **264 Hit** · 266 Aim
· 283 Equip Mode · **505 Hit Situational** · 759 Successful Hit · 1104 Reactive (skills only).

**Situational types** (`situational_type_value_id`): 506 Crouch · 507 Jump · 508 Sprint · 509
Backstab · 718 Block Breaker · **1270 Health-Below** · **1271 Health-Above**.

> ⚠ **Trap.** `situational_type_value_id` is populated on nearly every effect group row, including
> ones that are not situational at all — plain type-264 groups almost all read 509 (Backstab)
> because that is the column's default. **The field is only meaningful when
> `effect_group_type_value_id = 505`.** Filtering on `situational_type_value_id` alone will give
> you hundreds of false positives.

---

## 3. Cleanses

### What one is

Property **140, "Remove Effect"**. One effect row per category purged, sitting in its own effect
group. `base_value` is the quantity — in practice always **99** or 100, i.e. "all of them".

The category to purge comes from the effect's `property_value_id`, and the quantity from
`base_value`. `TgEffect.ApplyEffect` sees `m_nPropertyId == 140` and calls:

```
TgEffectManager::RemoveEffectGroupsByCategory(nCategoryCode, nQuantity)
```

⚠ The category ← `property_value_id` and quantity ← `base_value` mapping is inferred from the data
shape (the values are exactly `Poison`/`Ignite`/… and 99) plus the header comment on our
reimplementation. I did not read the UnrealScript for `ApplyEffect` — see §8.

### The count you want already exists and is being thrown away

`src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.cpp`
is **our code** (the shipped native is a stub at `0x10a6ef20`). It already maintains a local
`removed` counter as it walks `s_AppliedEffectGroups` backwards, and then discards it:

```cpp
int removed = 0;
for (...) { ...; removed++; }
if (removed > 0) Manager->bNetDirty = 1;
return removed > 0;          // ← the count dies here
```

This is the cheapest win in the whole document. `Manager->r_Owner` is the pawn that got cleansed;
`removed` is exactly "how many debuffs this wave stripped off this target".

### ⚠⚠ The one trap that will wreck your numbers

**Not every call to this function is a cleanse.** Two internal callers use it as a mechanism, not
as a player action:

- **Category 862 (Invulnerable)** — the function decrements `r_nInvulnerableCount` as a special
  case. That is refcount bookkeeping.
- **`RemoveEffectGroupsByCategory(431, 99)`** is used by
  `src/GameServer/TgGame/_effect_core/HitSituationalMitigation.cpp` as the *closing bracket of
  damage mitigation*. It fires on **every single damaging impact in the game**. If you instrument
  the function's entry point naively you will log a "cleanse" for every bullet that lands anywhere
  on the server.

  ⚠ **Check your branch base for this one.** `HitSituationalMitigation` arrived with the Killer
  Instinct fix (upstream PR #65) and is present on `upstream/master`, but **not** on the fork's
  `master` at the time of writing — the fork was 9 commits behind. So the 431 caller may or may not
  exist in your tree. It is a trap the moment you rebase onto upstream, so write the instrumentation
  as though it is there regardless.

Gate your instrumentation on the call arriving from the property-140 path, not on the function
being entered. If that is awkward to detect from inside, pass a flag from `ApplyEffect`, or
instrument at the `ApplyEffect` site instead and read the return value.

### Who actually cleanses

Player-equippable, from `gaa.db`:

| Device | Categories it removes |
|---|---|
| **Healing Wave** | Poison, Ignite, Disease, Slow, Stun, Power Pool Debuff, General Debuff |
| **Neutralize Wave** | Damage Reflect, Ability, Morale Buff, General Buff, Personal Damage Buff, Personal Shield, Shield Movement Penalty, Proximity Damage Buff, Proximity Shield, Regeneration, Regen Damage Penalty, Stim Boost, Stim Resistance, Self Heal, Power Pool Buff |
| **Sealed Systems** | Stun, Poison, Ignite, Additional Damage, Disease |
| **Healing Grenade** | Poison, Ignite, Disease |
| **Purity** | Poison, Ignite, Disease |
| **Detoxicant Device** | Poison, Disease |
| **Bionics** | Slow |
| Burst Nanite (dev) | Regeneration |

Note **Neutralize Wave strips buffs off enemies** — it is offensive cleansing. If "effective
cleanse" is going to be a single metric, it will be averaging two quite different actions.

### Counting it

**One Healing Wave cast produces seven separate calls per target hit** — one per category, each
its own effect group (eg 7956, 7957, 16651, 18931–18934). So:

- *used* = the wave fired.
- *effective* = `removed > 0` on at least one of the seven, for at least one target.
- *debuffs removed* = the sum of `removed` across all seven, across all targets.

These are three different numbers and they will diverge a lot. A wave that hits five clean
team-mates is "used, ineffective, 0 removed". A wave that hits one team-mate carrying poison and
burn is "used, effective, 2 removed".

Since waves are area effects, **decide up front whether your unit is the cast or the target**. I
would record per target and aggregate, because per-cast throws away the thing that makes the stat
interesting.

---

## 4. Triage Wave

Device **5808**. Two effect groups, and this is the whole story:

| Effect group | Type | Gate | Gives |
|---|---|---|---|
| **22392** | 264 Hit | none — always | Health 600 |
| **22375** | **505 Hit Situational** | **Health-Below 25%** | Health 600 **+ Power Pool 250** |

So:

- **Baseline:** every target hit gets 600 health, unconditionally.
- **Effective use:** the target was under 25% health, so 22375 *also* applied — a second 600 health
  (1200 total) **and all 250 power**.

**This answers the power question directly: the power restore lives only in the conditional group.**
A Triage Wave that hits nobody under 25% restores no power at all. That makes "did any power land"
a clean, unambiguous proxy for effective use — arguably a better signal than the health, because
the health is ambiguous between the two groups.

Application rule is **Stackable** (`application_value_id` 155), so the conditional half does not
displace or refresh the unconditional half; both land.

### Where the decision is made

`UTgDeviceFire::ShouldSituationalApplyEffect(ATgPawn* TargetPawn, UTgEffectGroup* EffectGroup)`
returns bool and is the single chokepoint for **every** 505 group in the game. It has everything
you need in its arguments: the target, and the group (which carries `m_nSituationalType` and
`m_fSituationalValue`).

> **RESOLVED — and the original advice here was wrong.** This section used to say that if the
> function were intact, hooking it would be the cleanest instrumentation point for both features.
> It is **not hookable at all**. The SDK dump gives its flags as `[0x00020002]`; bit `0x400`
> (`FUNC_Native`) is clear, so it is **pure UnrealScript bytecode**, neither an intact native nor a
> stripped stub. And in this build intra-UC calls do not route through `UObject::ProcessEvent` —
> ProcessEvent sees client RPCs, engine-timer dispatches, events, `super.` calls and native→script
> transitions, not a script function called from another script function. So the predicate is
> invisible to every hook we own.
>
> The correct approach, and the one the shipped implementation takes, is to **instrument a native
> downstream of the decision** — `TgEffect::CheckEffectBuffModifier` runs on every effect before its
> value is consumed and still knows the group, instigator, target and source device. Read the
> outcome, not the decision.

### Every HP-gated effect group in the game

There are only five, which makes this a small and testable surface:

| Effect group | Gate | Owner | Effect | Life |
|---|---|---|---|---|
**Read the calc method, never the bare `base_value`** — every row below stores a positive number and
the sign lives in `calc_method_value_id` (67 Add / 68 Increase +% / 69 Decrease −% / 70 Subtract):

| Effect group | Gate | Owner | Effect | Life |
|---|---|---|---|---|
| 22375 | Below 25% | **Triage Wave** | Health **+600** (67) + Power Pool **+250** (67) | instant |
| 16587 | Below 25% | skill **Group Heal Savior** (852) | Protection-Physical **+10** (67), GroundSpeed **+10%** (68) — buff on the team-mate | 5s |
| 16596 | Above 75% | skill **Killer Instinct** (836) | Protection-Physical **−10** (70) — debuff on the enemy | 3s |
| 26474 | Above 75% | skill **Combat Off-Hand Utility** (806) | Protection-Physical **−5** (70), GroundSpeed **−10%** (69) — debuff on the enemy | 5s |
| 27595 | Below 25% | zzHeavy Ion Sword (dev) | Health **−800** (70) — execute damage, not a heal | instant |

> **Combat Off-Hand Utility is not an anomaly — two earlier readings of it here were wrong, both
> from the same mistake.** This file first called it a suspected authoring leftover, then
> "CONFIRMED LIVE … buffing the enemy you poison … design intent still unexplained". Both read
> `base_value` without `calc_method_value_id`. It is calc **70 Subtract** and **69 Decrease −%** —
> a *debuff*, and the in-game tooltip states it plainly:
>
> > "Increases the effect radius of Combat Off-Hands. When you hit someone with a Combat Offhand and
> > they are over 75% health, the target's Protection is reduced by 5% and GroundSpeed by 10% for 5
> > seconds."
>
> So the live observation was right — it fires constantly on a poison build, lands on the target,
> `srcSkillId=336` — and only the interpretation was wrong. It is Killer Instinct's shape at lower
> numbers on a different weapon family, and there is nothing left to explain.
>
> Two data caveats behind the confusion. `asm_data_set_skill_group_skills.desc_msg_translated`
> carries only the first clause ("Increases the explosion radius of Combat Offhands"); the client
> tooltip carries both, so the DB description is **not** the full skill text and a mismatch against
> it means nothing. And a positive `base_value` on a protection effect says nothing about direction.

---

## 5. Group Heal Savior

Skill **852**, effect group **16587**, type **505**, gate **Health-Below 25%**. Grants
Protection-Physical +10 and GroundSpeed +10% for **5 seconds**. Application rule **Refresh** (836),
so re-triggering on an already-buffed target extends rather than stacks.

In-game description: *"When you hit a teammate with a group heal and his health is under 25%, you
provide the target with a 10% Protection and GroundSpeed bonus for 5 seconds."*

### How "with a group heal" is enforced

Via `required_skill_id = 252` on the effect group. Skill 252 is the **Group Heals** device family:

> Healing Wave · Healing Grenade · Protection Wave · Frenzy Wave · Power Wave · **Triage Wave** ·
> Purity

That is the mechanism — the skill effect group rides along on any hit, and `required_skill_id`
filters it to hits delivered by a device in that family. Same mechanism scopes Killer Instinct to
Recon Rifles (327).

> **CORRECTED 2026-08-04 (measured, instances 204/205/207/208).** The overlap this section
> originally asserted — Triage Wave triggering *both* its own conditional *and* Group Heal Savior
> from one impact — is **wrong**. Single-cast tests show **Triage Wave never procs Group Heal
> Savior**: a solo Triage rescue applies eg 22375 and no 16587; a solo Healing Wave rescue applies
> 16587 and nothing else. Healing Wave and Healing Grenade both proc it. Mechanism unestablished
> (the three devices carry identical family-252 linkage in the data); the behaviour is measured,
> not explained. Also measured: a skill-sourced instance's `m_nSourceDeviceInstId` reliably carries
> the **delivering** device's inventory id, so GHS triggers are device-attributable after all.

### Recording it

Dispatch site is `SubmitHitEffects(.., 1, 505, 1270)` — eSource **1** (skill), not 0. The predicate
is the same `ShouldSituationalApplyEffect`, which is **not instrumentable** (see the resolved note
in §4) — observe the applied effect group downstream instead, and distinguish the two features by
effect group id: 22375 Triage Wave, 16587 Group Heal Savior.

Attribution needs care: the *credited player* is the instigator (the medic), but the *target* is
the team-mate. Follow the existing convention in `TgEffect__TrackStats` — pet → owner resolution
via `r_Owner`, and skip self-effects (`Instigator == Target`, and the indirect deployable case
`Instigator->r_Owner == Target`). Group Heal Savior on yourself should almost certainly not count.

---

## 6. Boosts

These are structurally different from the three above: **there is no gate**. They are plain type-264
Hit groups with a lifetime.

| Device | Effect group | Life | Gives | Application rule |
|---|---|---|---|---|
| **Protection Boost** (2838) | 8964 | 10s | Protection-AOE +25, Protection-Ranged +25, Protection-Melee +25 | Newest Wins (156) |
| **Healing Boost** (2773) | 8690 | 10s | Health 200 | Strongest Wins (157), priority 2000 |
| **Fashion Boost** (7559) | 27646 | 30s | Health Max +20%, Power Pool Max +40, Effect Damage +15%, Effect Healing +15%, Power Pool Recharge +20% | — |

So "activation" is not a conditional question. What varies, and what is worth measuring, is
**reach and waste**:

- **How many targets did one activation land on?** A Protection Boost that catches five team-mates
  is doing five times the work of one that catches one. This is the number I would prioritise.
- **Was it wasted?** Two distinct kinds:
  - *Healing Boost on a full-health target* — the 200 health is clamped away. `TgEffect__TrackStats`
    already computes exactly this: `effectiveHeal = min(magnitude, fMissingHealth)`. If that comes
    out zero, the boost did nothing for that target. **You can get this for free from the existing
    heal path** without touching the effect system at all.
  - *Protection Boost on someone who then takes no damage for 10s* — genuinely useful, genuinely
    harder, needs a windowed lookback. Probably phase two.

⚠ **Protection Boost's three properties are an OR, not a sum.** `CalcAttackTypeProtection` is a
switch — exactly one attack-type axis ever applies per impact (AOE *or* Melee *or* Ranged, never
combined). So +25/+25/+25 is "+25 against whatever type this hit was", not +75. Do not report it as
75 anywhere.

The **Strongest Wins on Healing Boost** is worth a flag: per measurements taken on 2026-08-03,
application rule 157 behaves as **newest-wins in practice** — the priority value does not block a
later application, contradicting `TgEffectManager::IsStrongest`. So a second Healing Boost
overriding a first is expected behaviour, not a bug, and "boost overwritten before it expired"
would be a legitimate waste metric.

---

## 7. Where to hook

- **UC functions** → the existing `UObject::ProcessEvent` hook,
  `src/GameServer/Core/UObject/ProcessEvent/UObject__ProcessEvent.cpp`. It classifies once per
  unique `UFunction*` into a `DispatchTag` and caches on the pointer, so adding a tag costs one
  `strcmp` on first sight and a switch lookup thereafter. Add to the ladder in
  `ClassifyFunction`, not to a per-call `strcmp`.
- **Functions we already reimplemented** → just edit them. `RemoveEffectGroupsByCategory` is ours.
- **Intact natives** → a `HookBase` under `src/GameServer/<Subsystem>/<Class>/<Native>/`. Note the
  stub-vs-intact rule: if the native is a stub, `CallOriginal` is a no-op and does **not** chain to
  parent overrides — call the parent hook explicitly. Decision is per hook.

Two house rules that will save you a round trip:

- **Every new `.cpp` goes into `Makefile` in the same change.** It is an explicit source list, not
  a wildcard. Forgetting gives you an `undefined reference` at link time.
- **Diagnostics go on a dedicated named channel** — `Logger::Log("devusage", ...)`, never
  `GetLogChannel()` (that one drives the call-tree visualiser). Consolidate everything for this
  work onto **one** channel before asking anyone to capture logs.

### ⚠ Two poison protections, and the names lie

There are **two** poison-shaped protection properties on two different axes, and `TgProperties.h`
and `gaa.db` label them in opposite ways:

| Prop | Axis | Consulted by | `TgProperties.h` | `gaa.db` |
|---|---|---|---|---|
| **159** | status **category** (Poison, cat 303) | `CalcCategoryProtection` | `TGPID_PROTECTION_POISON` | "Protection - Biological" |
| **324** | damage **type** (Poison, type 897) | `CalcDamageTypeProtection` | `TGPID_PROTECTION_BIO` | "Protection - Poison" |

Whichever source you trust, the other looks like the error. The engine only ever sees the id, so
neither label is authoritative — the split is established from how the data groups them:

- `HUMAN BASE ATTRIBUTES` (device 864) moves **155/156/157/324** as a quartet, matching the four
  damage types exactly: Physical, Fire, Energy, Poison.
- Every device that grants 159 pairs it with *category* protections — Sealed Systems (159 with
  Disease/Stun/Ignite), Scorpion Shell, Perfect Target, Super Shell (159 with Ignite/Bleed).
- The Invulnerable Volume (eg 8698) raises **both in one group at different tiers**: 155/156/157/324
  and the attack types at 100, 159 and Ignite at 200.

Reach for **159** when you mean a poison DoT, **324** when you mean poison-typed damage. Same
comment now sits on both constants in `src/GameServer/Constants/TgProperties.h`.

Harmless today — `TgPawn__InitializeDefaultProps` seeds both identically (raw 0, min 0, max 1000)
and every consumer reads by id — but it is live the first time someone writes category code and
picks the constant by its name.

There is **no protection property of any kind for category 986 (Additional Damage)**. Ten status
categories have one; that one does not. So an Additional-Damage debuff can be cleansed but never
refused — which is why Sealed Systems strips the Pain Gun's +30% amplifier and cannot block it.

---

## 8. What I could not verify — check these first

1. ~~**Is `UTgDeviceFire::ShouldSituationalApplyEffect` intact or stripped?**~~ **RESOLVED: neither
   — it is pure UnrealScript** (`[0x00020002]`, no `FUNC_Native`), and intra-UC calls are invisible
   to `UObject::ProcessEvent` in this build, so it cannot be hooked. Instrument a native downstream
   of it. See the resolved note in §4.
2. ~~**How the health percentage is computed inside that predicate.**~~ **RESOLVED by measurement.**
   The worry was that the gates might use the integer-division pattern at `ApplyHit:1287` —
   `(TargetPawn.Health / TargetPawn.r_nHealthMaximum) > (90 / 100)`, where `90/100` folds to **0**
   and the left side is 0 unless the target is at *exactly* full health, which is why the
   anti-one-shot health cap only arms at 100%. The situational gates do **not** behave that way:
   Triage Wave's eg 22375 fires on real sub-25% rescues, and Combat Off-Hand Utility's above-75%
   gate fires constantly (instances 204/205/207/208/212). Both directions work. The `ApplyHit:1287`
   integer-division bug is still real and still only affects the health cap.
3. **The property-140 argument mapping** (category ← `property_value_id`, quantity ←
   `base_value`). Inferred from data shape and our own header comment, not read from source. Still
   open — `CleanseTracking` notes the same gap for the ordering of the 140 branch relative to
   `TgEffect.uc:115`, and degrades to "unattributed" rather than miscounting if the assumption is
   wrong.

I could not read the UnrealScript for any of these: `github.com/commonwealthga/ga-source` returned
404 unauthenticated from this machine and there is no `gh` CLI here. Anyone with repo access can
settle all three from `TgDeviceFire.uc` and `TgEffect.uc` in about fifteen minutes.

---

## 9. Suggested minimum viable set

If the goal is "mark effective vs ineffective use", the smallest thing that delivers real signal:

| Metric | Source | Cost |
|---|---|---|
| Debuffs removed, per device, per player | `removed` in `RemoveEffectGroupsByCategory` | trivial — the number already exists |
| Heal wasted on full-health targets | `effectiveHeal` clamp already in `TgEffect__TrackStats` | trivial — already computed |
| Triage Wave conditional triggers | eg 22375 applying (or its Power Pool restore landing) | native downstream of the gate |
| Group Heal Savior triggers | eg 16587 applying | same hook as above |
| Boost targets reached per activation | count distinct targets per effect-group application | moderate |

The first two need no new hooks and no answered questions. I would ship those first and treat the
situational ones as a second pass behind the §8 checks.
