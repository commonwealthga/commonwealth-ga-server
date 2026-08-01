# Damage / debuff / protection pipeline

Living map of the full PvP (and general) damage calculation: how outgoing damage is
built, how incoming damage is mitigated, and every modifier that plugs into the chain
(weapons, buffs, debuffs, skill-tree effects, device effects, armor, shields).

Branch: `weapon-damage-calculations`. This doc is the discovery deliverable — a map,
not a contract. Verify any address / offset / propId against current source before
relying on it; the effect-system rebuild is in progress and things move.

> **Status legend:** ✅ reimplemented & verified · ⚠️ partial / suspect · ❌ stubbed / missing · ❓ not yet traced

---

## 0. Trigger scenario — Recon "second shot" interaction

**Class:** Recon, full skill tree built. **Weapon:** Ballista sniper rifle (ranged).

Three skills acting in concert, plus the Ballista's own on-hit debuff:

| Skill / source | Stated effect | Side | When |
|---|---|---|---|
| **Killer Instinct** | Hit a target with >75% HP → lower their protections by 10% | Defensive (debuff on target) | on-hit, conditional |
| **Eagle Eye** | Various debuff-% increases, believed to interact with the Ballista debuff (amplify the protection-reduction magnitude/duration) | Defensive (debuff amplifier) | on-hit |
| **Super Sharp Shooter** | Hit a target with ranged rifle → +5% ranged damage for 3 s | **Offensive (self-buff on attacker)** | on-hit, 3s window |
| **Ballista debuff** | On hit, reduces target protections for a time | Defensive (debuff on target) | on-hit, timed |

**Observed:** Shot 1 does X. Given the Ballista debuff (+ Killer Instinct / Eagle Eye
protection reductions on the target, and Super Sharp Shooter's +5% self-buff), shot 2
should do X + (some computable amount) more. **Odd behaviour is seen on the second
shot** — the increase is not what the numbers predict.

**Why this hits both halves:** the second-shot bonus is the *product* of an offensive
self-buff (Super Sharp Shooter, prop-65-ish ranged damage, 3s) and defensive
protection-reduction debuffs on the target (Killer Instinct 10%, Eagle Eye amplifier,
Ballista timed). A bug in stacking order, displacement, double-apply, or the 3s / debuff
timing window on *either* side would corrupt the shot-2 number.

**Hypotheses to test (fill/refute during trace):**
- [ ] Super Sharp Shooter self-buff not applying/refreshing on shot 1 → shot 2 offense flat.
- [ ] Protection-reduction debuffs displacing each other (newest-vs-strongest) instead of stacking → shot 2 defense reduction wrong.
- [ ] Eagle Eye amplifier applied to attacker's buff registry instead of the target's debuff magnitude.
- [ ] Debuffs double-applying on shot 2 (re-clone without reversing shot-1 clone).
- [ ] Timing: 3s SSS window vs Ballista debuff lifetime mismatch → one expires before shot 2.
- [ ] **Killer Instinct HP-gate timing** — KI only fires when target >75% HP. Shot 1 (target 100%) applies it; shot 2 (target now <75% after a sniper hit) does NOT re-apply, but shot-1's KI debuff is still live (3 s). Whether this is a bug or intended depends on the expected model. ← evaluate first.

### Measured test (2026-07-31) — Ballista OC vs bare target (no skills, no armor)

Attacker: Recon, full tree (Recon Rifle Range/Power/Damage/Eff-Range/Accuracy, **Eagle Eye,
Killer Instinct, Sureshot, Super Sharpshooter**), Ballista OC. Target: base attributes only —
**Physical protection (155) = 30**, Ranged protection (218) = 0 (HUMAN BASE ATTRIBUTES item 864,
unremovable: +30 Phys, +1000 EMP-Stun, +1000 EMP-Burn).

| Shot | dealt | mitigated | raw (=dealt+mitig) | mitigation % | implied Phys prot |
|---|---|---|---|---|---|
| 1 | 1170 | 430 | 1600 | 26.9% | **30** (base) |
| 2 | 1548 | 116 | 1664 | 6.97% | **≈10** |

### A/B controlled test (2026-07-31) — EXACT model (all 6 shots reconcile to the unit)

Two extra runs (drop one skill each) pinned everything. **mit% = Physical protection (clean 1:1)**,
base = 30, raw shot-1 = 1600, raw shot-2 = 1664:

| Build | Shot | protection | mit (pred=meas) | dealt (pred=meas) |
|---|---|---|---|---|
| Baseline | 1 | 30 − 3.1 (KI self-leak) = 26.9 | 430 | 1170 |
| Baseline | 2 | 30 − 13 (Ball+EE) − 10 (KI) = 7 | 116 | 1548 |
| No Eagle Eye | 1 | 30 − 3.1 = 26.9 | 430 | 1170 |
| No Eagle Eye | 2 | 30 − 10 (Ball) − 10 (KI) = 10 | 166 | 1498 |
| No Killer Instinct | 1 | 30 (clean) | 480 | 1120 |
| No Killer Instinct | 2 | 30 − 13 (Ball+EE) = 17 | 283 | 1381 |

**Confirmed mechanics:**
- **Mitigation formula: mit% = Physical-protection value, 1:1.** (Earlier "mit% ≈ P−3" was an artifact of KI's self-leak contaminating the baseline shot 1. Corrected.)
- **Offense:** raw 1600 → 1664 = +4% ⇒ Super Sharpshooter on-hit +5% (eg 26675). ✓
- **Ballista debuff (eg 18975, type 264 Hit):** base −10, **amplified by Eagle Eye to −13 (+30% potency, prop 376 — matches eg 17244's value).** ✓
- **Killer Instinct (eg 16596, type 505 Hit-Situational):** −10, **NOT amplified by Eagle Eye** (baseline & No-EE both put KI at exactly −10).

**THE ANOMALY:** Eagle Eye's potency amplifies the **device** debuff (type-264) but **not** the **skill**
debuff (type-505). Both are class-80 prop-155 −10 — the only difference is the apply dispatch. ⇒ type-505
(Hit-Situational) application does not route through the potency-scaling (`CheckEffectBuffModifier` /
`ConvertPropToPropList bUsePotencyModifier=1`) path that type-264 (Hit) uses.

**SECONDARY QUIRK:** Killer Instinct leaks ~3 protection points onto its **own triggering shot** (shot-1
protection 30→26.9 only when KI slotted; Ballista leaves shot 1 clean). Consistent with type-505 applying
earlier in impact processing than type-264.

**Corrected earlier mistake:** the baseline-only pass concluded "Eagle Eye is inert" — WRONG. The A/B test
shows EE works on the Ballista debuff and only fails on the skill debuff. Controlled test > single-point inference.

### Design intent (confirmed with user 2026-07-31)
Eagle Eye's potency is **meant to amplify weapon/device debuffs ONLY, not Killer Instinct.** So the
type-264-gets-potency / type-505-doesn't behaviour is **correct**, not a bug. The A/B numbers show our
server reproduces it faithfully.

### Trace result — WHY Eagle Eye scopes to weapon/device (confirmed from code)

Emergent from **skill-scoped buff × origin resolution** — no special-casing:

1. **Eagle Eye (eg 17244, type 261 Equip)** applies at `ReapplyCharacterSkillTree` (only type-261 applies
   at equip; 264/505/759 are deferred to combat events — RCST line ~281). Its prop-376 potency buff registers
   in `m_EffectBuffInfo` **scoped to `nReqSkillId = 327` (Recon Rifles)** = the effect group's `required_skill_id`.
2. **Ballista debuff (device effect):** `OriginResolver` reads `m_nSourceDeviceInstId/SkillId` from the
   weapon Impact → **skill 327**. `CheckEffectBuffModifier` → `GetBuffedProperty(nReqSkillId=327,
   bUsePotencyModifier=1)` → **matches** Eagle Eye's skill-327 entry → −10 → −13 (+30%). ✓
3. **Killer Instinct (skill effect, `m_bSkillEffect`):** skill-sourced, **no device origin**
   (`m_nSourceDeviceInstId=0`) → potency query `skillId=0`. Buff-match rule (confirmed in
   `TgPawn__ApplyBuff.cpp`: *stored>0 only matches same skillId*) ⇒ Eagle Eye's stored 327 ≠ query 0 →
   no potency → stays −10. ✓

**Conclusion: correct intended behaviour, faithfully reproduced.** The "weapon-only" scoping is not a
rule anyone wrote — it's that Eagle Eye's potency is skill-scoped (327) and only device-sourced effects
carry that skill as origin.

### Remaining open item — KI self-shot leak
KI bleeds ~3 protection points onto its own triggering shot (type-505 applies earlier in `SubmitHitEffects`
than type-264). The ordering is **UC bytecode** — not in the binary or our natives — so it can't be pinned
by static reading. Needs a log capture of the on-hit apply sequence, or the original-server behaviour, to
classify as bug-vs-faithful. Low impact (~4% of shot 1). **Not blocking.**

### Confirmed DB identities

**Name source:** `E:\GA_LOCAL\gaa.db` — only `asm_data_set_skill_group_skills` / `_ranks` carry
translated names (items + device modes are blank there). Skill→effect-group via
`asm_data_set_skill_effect_groups`. Effect defs verified in gaa.db.

> ⚠️ Correction from first pass: `required_skill_id` on these groups is **327 = "Recon Rifles"**
> — the *weapon-proficiency gate*, NOT the granting skill. The granting skill IDs (834/836/693)
> come from `asm_data_set_skill_effect_groups.skill_id`. Earlier 281/327 labels were the gate skills.

| Skill | skill_id | Effect group | Mechanic (verified) |
|---|---|---|---|
| **Killer Instinct** | **836** | eg **16596** | type **505** Hit-Situational, situational **1271** `value=75.0` (**target HP >75%**), **class 80** direct-prop, prop **155** Protection-**Physical** **−10** (calc 70 SUB), lifetime **3 s**. Physical-only, not 217/218/219. |
| **Eagle Eye** | **834** | eg **17244** | type **261 PASSIVE (equip)**, situational 509, **class 157 buff**, prop **376 Effect Potency +50%** (+30/+30/+50 tiers) & prop **208 Device Range +20%**. The **debuff amplifier** — potency 376 feeds `CheckEffectBuffModifier`'s `bUsePotencyModifier=1`. Always-on when equipped. |
| **Super Sharpshooter** | **693** | eg **26675** (+ passive **16518**) | 26675: type **759** self-buff, **class 157**, prop **214 Damage-Range +5%**, lifetime **3 s** (matches tooltip), refresh-on-hit. 16518: type **261 passive**, prop **214 +10%** + 232 attack-rate +5% + 256 acc-correction +20%, always-on. |
| **Recon Rifles** (gate) | 327 | — | weapon proficiency; the `required_skill_id` on all the above. |
| **Ballista Sniper Rifle** | 281 | — | weapon skill; no `skill_effect_groups` — effects live on the device. |
| **Ballista OC** (device) | **device 2110**, mode 2618 | eg 6164 / 6166 / **18975** | **6164** = base damage prop 51 Health **−585** (class 181), situational 506. **6166** = aim/scope prop 49 ×0.75 (class 80). **18975** = **on-hit debuff: prop 155 Protection-Physical −10, 5 s, class 80, category 986, situational 509.** |

**Name resolution note:** `gaa.db.asm_data_set_msg_translations` (msg_id→text) is the working name map.
"Ballista OC" = msg_id 61213 → blueprint `override_name_msg_id` → `created_item_id 2110` → device 2110.

---

## 5. The scenario's interaction model (offense × defense on shot 2)

**Base damage** = Ballista eg 6164, prop 51 Health **−585** (class 181 TgEffectDamage). On each shot it is:
1. **Scaled offensively** in `CheckEffectBuffModifier` by the attacker's buff registry
   (ConvertPropToPropList expands prop 65 → {385 Output Mod, 214 Range-Dmg, 372 Physical-Dmg, 376 Potency, …}).
2. **Mitigated defensively** by the target's **Physical protection (prop 155)** via `CalcProtection`.

**Attacker-side offense (prop 214 Range-Dmg):**
- Super Sharpshooter **passive +10%** (eg 16518) — always on, affects **both** shots equally → NOT part of the shot-2 delta.
- Super Sharpshooter **on-hit +5%** (eg 26675, 3 s) — applied on shot 1, **live for shot 2** → **+5% is the offensive shot-2 delta.**

**Target-side defense (prop 155 Physical protection reduced):**
- Ballista debuff eg 18975: **−10, 5 s, category 986** — applied shot 1, live for shot 2.
- Killer Instinct eg 16596: **−10, 3 s, category 302**, **only if target >75% HP at hit.** Applies shot 1 (target 100%); on shot 2 the target is likely <75% so KI does **not** re-apply, but shot-1's KI is still live.
- Different categories (986 vs 302) ⇒ **should stack to −20** on shot 2, not displace.

**Eagle Eye (prop 376 Potency +50%, passive):** intended to amplify the applied debuff magnitude
(−10 → −15). **BUT** both debuffs are **class-80 direct-property** effects, and potency scaling only
happens inside `CheckEffectBuffModifier` (`bUsePotencyModifier=1`). **Open question: do class-80
protection-debuff effects even route through `CheckEffectBuffModifier` on apply?** If not, Eagle Eye
never touches them — which would exactly explain "Eagle Eye should interact with the Ballista debuff
but the behaviour is odd."

### Refined hypotheses for the odd second shot (ranked)

1. **Eagle Eye potency (376) not applied to the class-80 protection debuffs.** If class-80 direct-prop apply bypasses `CheckEffectBuffModifier`, the −10s never become −15 → shot-2 defense weaker than expected. **← top suspect, matches the user's framing.**
2. **Ballista (cat 986) and Killer Instinct (cat 302) debuffs not stacking** — if the apply/displacement logic collapses them (same prop 155, same −10), shot 2 sees −10 not −20.
3. **Class-80 debuff revert imbalance** — direct `m_fRaw -= delta` reversal on a 3 s vs 5 s pair; if KI (3 s) reverts while Ballista (5 s) is still live, or a double-apply on shot 2's re-clone, prop 155 drifts (cf. Armor.cpp / ProcessReactiveSkillBasedEffectGroup idempotency notes).
4. **Killer Instinct HP-gate evaluation** — if HP is read post-damage, KI may fail to apply even on shot 1.
5. **Super Sharpshooter on-hit +5% (eg 26675) not applying/refreshing** — offense flat between shots.

**Second-shot arithmetic inputs (from the above):**
- Offense shot-2 gain vs shot-1: Super Sharpshooter's **+5% ranged (prop 214)**, 3 s, applied on shot-1, live for shot-2. (Its +10% passive is always-on and affects *both* shots equally, so it's not part of the *delta*.)
- Defense shot-2 gain: Killer Instinct **−10 Physical protection** on the target from shot-1 (3 s) — **only if the target was >75% HP when shot-1 landed**. Eagle Eye's **+50% potency** should amplify the applied debuff magnitude *if* KI's class-80 effect routes through the potency-scaling path (**open question — see below**).
- Any Ballista *device* debuff is additional and still unquantified (device_id needed).

### Situational / type enum decode (from DB + code)
- effect_group_type: **264** = Hit · **505** = Hit-Situational · **759** = skill self-buff · 263 = DeviceFiring · 266 = Aim · 283 = Equip-Mode · 261 = Equip · **1104** = Reactive-Skill (Aegis-style, lifetime=0 only).
- situational_type: **509** = on ranged hit (generic) · **1271** = target HP **above** `situational_value`% · **1270** = HP **below** `situational_value`% · 506/507/508 = other attack conditions (TBD).

### The three application paths on ONE shot (all UC bytecode, calling our natives at leaves)

```
TgDeviceFire.SubmitHitEffects(instigator, impact, eSource, nType, nSituationalType)   [UC]
  ├─ nType 264  Hit            → target: base damage (TgEffectDamage 181) + plain on-hit debuffs
  ├─ nType 505  Hit-Situational→ ShouldSituationalApplyEffect(target, eg)   [UC, evals 1271 HP>75 etc.]
  │                                └─ if pass → SubmitEffect → target.ProcessEffect → Killer Instinct −10 prot
  └─ nType 759  skill self-buff → GetSkillBasedEffectGroup [OUR native] → attacker.ProcessEffect
                                   └─ Super Sharp Shooter +5% ranged, via ApplyBuff [OUR native]

  every applied effect's magnitude → CheckEffectBuffModifier [OUR native] → GetBuffedProperty (intact)
```

**Key architectural takeaways for the bug hunt:**
1. SSS (offense) and KI/Ballista (defense) ride **different `nType` dispatch buckets** in the same `SubmitHitEffects` call — a bug in one bucket's iteration (e.g. `GetSkillBasedEffectGroup`/`GetEffectGroup` `nIndex` handling) breaks only that half of the second-shot delta.
2. SSS is a **class-157 buff** (registry, refreshes/displaces per `application_value`), KI is a **class-80 direct-prop debuff** (raw `m_fRaw` write on target) — they revert by completely different paths, so "second shot" stacking/refresh semantics differ between them.
3. `CheckEffectBuffModifier` is the single offensive funnel; if Eagle Eye is a potency (376) buff, it only takes effect through that native's `bUsePotencyModifier=1` path.

---

## Scope of this pass: OFFENSIVE HALF

How outgoing damage is *built* — from the weapon's base roll up to the value handed
to delivery (`ApplyDamage`). The defensive half (CalcProtection / armor / shields /
TakeDamage) is a later pass; stubbed here as the hand-off boundary.

---

## 1. Offensive chain (current architecture)

```
WEAPON FIRE  (TgDeviceFire, per device + fire mode)
  │  builds/caches an effect-group TEMPLATE with DB defaults:
  │  m_fBase / m_fMinimum / m_fMaximum / lifetime / interval
  ▼
GetEffectGroup / ApplyFireModeSetup            [template cache on the firing device]
  │
  ▼  per impact
CloneEffectGroup                               [fresh UTgEffectGroup + fresh UTgEffect children]
  │  NOTE: no longer pre-scales m_fBase (docs/claude/effect-system.md is STALE on this).
  │  Scaling now happens downstream in CheckEffectBuffModifier.
  ▼
CheckEffectBuffModifier(effect, &NewValue)     [THE offensive scaling point]
  │  resolves SOURCE pawn (attacker, or deployer behind a deployable)
  │  queries source pawn's buff registry via the intact GetBuffedProperty:
  │      GetBuffedProperty(BUFF_PAWN, effProp, cat, skill, devInst, potency=1, base, &out)
  │  ConvertPropToPropList expands effProp → the modifier set that applies:
  │      damage → {65 dmg, 385 output-mod, per-attack-type 212/214/321, 376 potency}
  │      heal   → {330, 385}   (query normalized to prop 51)
  ▼
scaled outgoing value  ──► DELIVERY: ApplyDamage(dmg, instigator, attackType, damageType, impact, category)
                                          └─► (DEFENSIVE HALF — later pass)
```

### Where the buff-registry modifiers come from

`GetBuffedProperty` reads the aggregated `m_EffectBuffInfo` on the source pawn. Entries
are registered by `ATgPawn::ApplyBuff` from every offensive-modifier source:

| Source | Registered by | Notes |
|---|---|---|
| Skill tree | RCST (ReapplyCharacterSkillTree) — `lifetime=0` clones baked at char load | stripped on death, must be re-run on revive |
| Rolled weapon mods | `Inventory::ApplyRolledModEffects` | device-scoped (by inventory id) |
| Armor mods | `Armor.cpp` | mostly defensive (HP/protection) but registers via same ApplyBuff path |
| Pet damage | `CheckOwnerPetBuff` | owner-side; do NOT also bridge prop 350 → pet 65 |
| Active buff effects | `TgEffectBuff` (class_res_id 157) apply/remove | HUD-slot buffs |

---

## 2. Key files (offensive half)

| Stage | File | Status |
|---|---|---|
| Fire template build | `src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.cpp` | ❓ |
| Fire mode setup | `src/GameServer/TgGame/TgDeviceFire/ApplyFireModeSetup/TgDeviceFire__ApplyFireModeSetup.cpp` | ❓ |
| Effect-group clone | `src/GameServer/TgGame/TgEffectGroup/CloneEffectGroup/TgEffectGroup__CloneEffectGroup.cpp` | ✅ (rebuilt) |
| Effect clone | `src/GameServer/TgGame/TgEffect/CloneEffect/TgEffect__CloneEffect.cpp` | ✅ |
| **Offensive scaling** | `src/GameServer/TgGame/TgEffect/CheckEffectBuffModifier/TgEffect__CheckEffectBuffModifier.cpp` | ✅ (rebuilt, stub→native) |
| Pet damage buff | `src/GameServer/TgGame/TgEffect/CheckOwnerPetBuff/TgEffect__CheckOwnerPetBuff.cpp` | ✅ |
| Buff registration | `src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.cpp` | ✅ |
| Skill re-apply | `src/GameServer/TgGame/TgPawn_Character/ReapplyCharacterSkillTree/…` | ⚠️ |
| Rolled mods | `src/GameServer/Inventory/Inventory.cpp` | ⚠️ |

### Intact natives leaned on (do NOT reimplement)
- `GetBuffedProperty` @ `0x109d7ff0` — the 3-layer buff formula.
- `ConvertPropToPropList` — per-effect propId → modifier-list expansion.

---

## 3. Property / prop-id crib (offensive-relevant)

| propId | Meaning | Layer |
|---|---|---|
| 65 | Damage | additive/mult on outgoing damage |
| 385 | Output Mod | multiplicative, routes through GP layer (`srcType=OTHER`) |
| 376 | Effect Potency | added when `bUsePotencyModifier=1` |
| 330 | +Heal | heal output |
| 350 | Pet Damage | owner-side pet buff |
| 212 / 214 / 321 | per-attack-type damage mods | expanded by ConvertPropToPropList |

_(Expand as the trace confirms which props the trigger scenario actually touches.)_

---

## 4. Open questions / to trace

- [ ] Exact `GetBuffedProperty` 3-layer formula (ITEM vs OTHER vs GP composition order) — needs Ghidra decompile of `0x109d7ff0`.
- [ ] `ConvertPropToPropList` damage expansion — full member list + whether it's additive-then-mult or mult-chain.
- [ ] Where the base damage *roll* (min/max) happens and whether crit/falloff apply before or after buff scaling.
- [ ] `attackType` / `damageType` routing — which per-type mod (212/214/321) maps to which weapon.
- [ ] Stale-reference cleanup: `.planning/effect-buff-property-canonical.md` (referenced in code, does not exist).

---

## 6. Weapon base damage + mod layering (validated on 2 weapons)

**Displayed tooltip "Damage" = device base × (1 + Output Mod 385).** Verified:
- Ballista OC: device 2110 base **585** × 1.75 = 1023.75 → tooltip **1023** ✓
- Raven SMG OC: device 6069 base **75** × 1.75 = 131.25 → tooltip **131** ✓

**Two mod layers (the `[dddddd]` roll code = 6 mod letters):**
1. **Base-weapon mods (green lines)** — `asm_data_set_blueprint_item_mods`, one row **per quality tier**
   (Common 1165 / Uncommon 1164 / Rare 1163 / Epic 1162), applied **cumulatively up to the weapon's
   rolled quality**. An Epic weapon = Common+Uncommon+Rare+Epic. Both OC rifles:
   Common = Output Mod (385) +75%, Uncommon+Rare+Epic = Range-Dmg (214) +4% ×3 = **+12%**.
2. **Mod-slot mods (blue lines)** — separately socketed (e.g. Damage 65 **+9%**), per-instance,
   applied via `Inventory::ApplyRolledModEffects`.

**Trace recipe for any OC weapon:**
`msg_translations.message='<Name>'` → `blueprints.override_name_msg_id` → `created_item_id` →
device of same id → mode's base-damage eg (prop 51, class 181) = base; `blueprint_item_mods`
(cumulative by quality) = green mods; socketed = blue mods.

**Consequence for the damage calc:** the headline number is only base×OutputMod. The +12% Range (214),
+9% Damage (65), and all skill buffs are class-157 buffs applied **at hit time** in `CheckEffectBuffModifier`
(scaling the raw base, e.g. 585 / 75) — NOT on top of the displayed 1023 / 131. Whether Output Mod (385)
is re-applied at hit time (double-count) or only once is still to verify.

> Tooling caution: never reuse one sqlite cursor for an inner query inside a `for row in cursor.execute()`
> loop — it resets the outer result set and silently drops rows. (Cost me a wrong "no +12% Range" call.)

## 7. Defensive half — passive protection catalogue (player skills)

Scope: **passive skill-tree protection (type 261, always-on when allocated) + armor.** Conditional/buff
protections (Stealth Protection, Aegis Armament, Protection Boost/Wave, etc.) are deferred. Source: `gaa.db`,
player trees only (skill_group_id 155–163; excludes device-container/base-attribute groups).

**Class → tree map** (`SkillTreeCatalog.cpp`): Balanced=155 (all classes) · Medic 156 Healer/157 Poison ·
Assault 158 Tank/159 Destroyer · Recon 160 Infiltration/161 Marksman · Robotic 162 Engineer/163 Drones.

**Protection axes** — two apply to a weapon hit, must be combined by `CalcProtection`:
- **Damage-type** (from weapon damage type): Physical 155 / Fire 156 / Energy 157
- **Attack-type** (from delivery): Melee 217 / Ranged 218 / AOE 219
- **Status/CC** (not damage): Stun 163 / Knockback 233 / Slow 158 / Sleep 168 / EMP-Stun 235 / EMP-Burn 328 / Ignite 266 / Poison 324 / Bleed 371 — catalogued but irrelevant to weapon-damage numbers.

| Tree | Skill (id) | Protection (type 261 passive) |
|---|---|---|
| **Balanced** | Passive Protection (533) | +4 Physical |
| Balanced | Advanced Passive Protection (676) | +5 Physical |
| Balanced | Super Agent (677) | +4 Physical |
| **Assault/Tank** | Heavy Armor (767) | +10 Melee / +10 Ranged / +10 AOE |
| Assault/Tank | Built Tough (768) | +15 AOE |
| Assault/Tank | Built Truck-Tough (769) | +15 Ranged |
| Assault/Tank | Built Brick Wall Tough (860) | +4 Ranged / +4 AOE / +15 Stun / +15 Knockback |
| Assault/Tank | Super Tank (546) | +5 Ranged / +5 AOE |
| Assault/Tank | Assault Melee I/II/III (888/889/890) | +10 / +7 / +6 Melee |
| **Medic** | Super Healer (703) | +8 Ranged / +8 AOE |
| Medic | Battle Medic (810) | +4 Melee / +4 Ranged |
| Medic | Medic Melee I/II/III (891/892/893) | +10 / +7 / +6 Melee |
| **Recon/Infil** | Athletic Dodge (825) | +10 Melee / +10 AOE |
| Recon/Infil | Recon Melee I/II/III (894/895/896) | +10 / +7 / +6 Melee |
| **Robotic** | Cyber Armor (791) | +8 Ranged / +8 AOE |
| Robotic | Super Supporter (798) | +5 Melee / +5 Ranged / +5 AOE |
| Robotic | Cybernetic Speed (906) | +15 Slow / +15 Knockback |
| Robotic | Robotic Melee I/II/III (885/886/887) | +10 / +7 / +6 Melee |

**Deferred (conditional/buff — not passive):** Stealth Protection (599, +8 Phys while stealthed, type 1104),
Aegis Armament (913, +25 Phys while shield up, type 1104), Group Heal Savior (852, +10 Phys situational,
type 505), Super Engineer (714, +5 Phys on-hit, type 264), Combat Off-Hand Utility (806, −5 Phys, type 505).
Killer Instinct (836) is the −10 enemy debuff (offensive), not self-protection.

**Notes:**
- Base Physical protection = 30 (HUMAN BASE ATTRIBUTES item 864, all characters). Balanced tree adds up to +13 Physical for anyone.
- Every class has a **Melee I/II/III** line (+10/+7/+6 = +23 cumulative). Melee is the most-invested axis; Ranged/AOE come from the tank/armor nodes above.
- Armor pieces contribute 217/218/219 via `Armor.cpp` (see §1) — combines with these skill values.
- **Open:** how damage-type (155) and attack-type (218) protection combine into one mit% — the `CalcProtection` formula (next).

## 8. CalcProtection — the mitigation formula (UC bytecode, not in binary)

`UTgEffectGroup.CalcProtection` and its family (`CalcDamageTypeProtection`, `CalcAttackTypeProtection`,
`CalcCategoryProtection`) are **UnrealScript bytecode** — the SDK wrappers dispatch via `ProcessEvent`
on `Function TgGame.TgEffectGroup.CalcProtection`, and repo comments cite `TgEffectGroup.uc` source lines.
**Not decompilable via Ghidra** (not in the exe); the formula lives in `TgGame.u`. No `.uc` source in-repo.

**Flow:**
```
TgEffectDamage.ProtectionModifier (UC, line ~165)         [damage-side entry; sets bSubmitMitigation]
  └─ CalcProtection(Target, nProtectionType, nDeviceRating, fValue, bSubmitMitigation, &fPercReduction)
       ├─ CalcDamageTypeProtection   damage-type axis (155/156/157)
       ├─ CalcAttackTypeProtection   attack-type axis (217/218/219)
       └─ CalcCategoryProtection     category axis
     bSubmitMitigation=true → TgEffectManager.SubmitMitigationDamage (OUR native) → shield HP absorb
```
Takes **`nDeviceRating`** (weapon attack_rating, =100 for both OC weapons) → reduction is protection-vs-rating,
not protection alone.

### EXACT formula (from decompiled UC — `github.com/commonwealthga/ga-source`, `unrealscript/TgGame/Classes/`)

Single axis — `TgEffectGroup.CalcProtection` (TgEffectGroup.uc:745):
```
nProtection     = int( FClamp(prop.m_fRaw, m_fMinimum, m_fMaximum) )   // integer-floored, clamped
fPercProtection = FMax(0, nProtection / nDeviceRating)                 // reduction fraction
fNewValue       = fValue - (fValue * fPercProtection)  = fValue × (1 − protection/rating)
```

Axis chaining — `TgEffectDamage.ProtectionModifier` (TgEffectDamage.uc:302), applied to the running value
in order, so **axes MULTIPLY (not add):**
```
final = raw_damage
      × (1 − categoryProt   / rating)   // by m_nCategoryCode → elemental/status prop; NO-OP for cat 302
      × (1 − damageTypeProt / rating)   // by m_nDamageType: 113→155 Phys, 115→156 Fire, 116→157 Energy, 897→324 Poison
      × (1 − attackTypeProt / rating)   // by m_eAttackType: 1→217 Melee, 2→218 Ranged, 3→219 AOE — ONLY if cat ∈ {302,963}
rating = FiredDevice.GetAttackRating()
```
NOTE: the target's prop-316 "Additional Damage Taken" multiplier is applied BEFORE this (see §10), not after.
```
```

**Validated:** Ballista (Phys 155=30, Ranged 218=0, cat 302, rating 100): `1600 × (1−0) × (1−0.30) × (1−0) =
1120` = clean No-KI shot-1 dealt. ✓ (baseline 1170 = KI self-leak).

**Consequences:**
- **Multiplicative across axes** — Physical 30 + Ranged 20 @ rating 100 = 0.70×0.80 = 0.56 → 44% mit (NOT 50%).
- **Rating penetrates** — reduction per axis = protection/rating; rating 200 halves a given protection's effect. Both OC weapons = rating 100.
- **Integer-floored & clamped**; attack-type axis only for normal weapon categories (302/963); elemental/status damage uses the category axis instead.
- Category axis map (CalcCategoryProtection): 303→159 Bio, 304→158 Slow, 305→160 Disease, 378→163 Stun, 431→168 Sleep, 875→233 Knockback, 653→235 EMP-Stun, 719→266 Ignite, 1016→371 Bleed, 921→328 EMP-Burn.

**Decompiled-source repo:** `github.com/commonwealthga/ga-source` — `unrealscript/` = full UC source (authoritative for
bytecode functions like this), `decompiled/` = Ghidra C++ of the exe. Use UC source for any `.uc`-cited function.

### Per-hit health cap (anti-one-shot) — `s_bApplyHealthCap`

A single hit on a **full/near-full-HP player** cannot drop them below **10% of max HP**; excess is clamped so
they land exactly at the 10% floor. Strictly per-hit.

- **Arm** — `TgDeviceFire.ApplyHit:1289`: at hit start, if `Health/maxHP > 0.90` → `s_bApplyHealthCap = true`.
  (Decompile shows integer-style literals; threshold is "exactly full" or ">90%" — either way, fresh target arms it.)
- **Enforce** — `TgEffectDamage.ProtectionModifier:174-199`, **non-bot pawns only**, flag set:
  `floor = ceil(maxHP*10%)`; if `damage > (Health-floor)` clamp `damage = Health-floor`; if already ≤floor, `damage = 0`.
- **Reset** — `TgDeviceFire.ApplyHit:1517`: `s_bApplyHealthCap = false` at hit end.

**All must hold to bite:** target is a player (bots exempt), was at/near full HP when hit, AND the post-mitigation
damage would cross the 10% floor. Otherwise no-op.

**Implications:** one-shots are prevented only *from the top* — a fresh target always survives the first blow with
≥10%; a target already <~90% has no cap and can be killed outright. Bots can be one-shot.

**Did NOT affect the §0 Ballista test:** shot 1 armed the flag (full HP) but didn't bite (1120/1170 reconciled with
the protection formula → max HP high enough); shot 2 was never armed (target <full after shot 1). All 6 shots matching
the formula is itself proof the cap stayed dormant.

## 9. Armor (defensive layer)

Scope: current server seeding (`gaa.db`). Skill protection is §7; this is the armor layer. Separate from
device equip slots (ES1–24, see `equip-slots.md`) — armor uses **7 group-126 "enhancement" slots**.

**Slots + per-slot baseline egid:**
| Slot ID | Piece | Baseline egid |
|---|---|---|
| 1130 | Head | 23173 |
| 1132 | Hands | 23289 |
| 1133 | Chest | 23224 |
| 1136 | Arms | 23237 |
| 1139 | Legs | 23302 |
| 1142 | Feet | 23315 |
| 1143 | Shoulder | 23211 |

**Per-piece structure** — `ga_players_inventory.mod_effect_group_ids` CSV = **1 baseline egid + 6 rolled mods**:
- **Baseline** (all 7): prop **390 Health-Mod +10%** (class 157) → **+70% Max Health** for a full set.
  *(Verified: base 1300 HP → 2210 with full armor = ×1.70; matches Armor.cpp's regression note.)*
- **6 rolled protection mods**, each **+0.5** of one stat. Player-facing **letter notation** (BASE 3 + MOD 3 —
  base-game split, now flattened into one stored CSV; no field distinguishes BASE vs MOD anymore):
  | Letter | Stat | egid | Each |
  |---|---|---|---|
  | **M** | Melee (217) | 24170 | +0.5 (class 80) |
  | **R** | Ranged (218) | 24165 | +0.5 (class 80) |
  | **A** | AOE (219) | 24083 | +0.5 (class 80) |
  | **N** | Max Health (412) | 24074 | +0.5% (class 157) |
  - e.g. `MMM MMM` = +3.0 Melee; `NNN RRR` = +1.5% Max Health + 1.5 Ranged. Only {M,R,A,N} exist in the live pool.
  - 7 slots = Head/Shoulders/Chest/Arms/Hands/Legs/Feet (slot naming is cosmetic; protection math is per-piece letters).

**Full-set contribution:**
- **+70% Max Health** — the dominant defensive value from armor.
- **Protection:** up to 7 pieces × 6 mods × 0.5 = **+21 of one attack-type axis** (Ranged/Melee/AOE) if fully min-maxed one direction. Feeds props 217/218/219 → the **attack-type axis** of `CalcProtection` (multiplicative — §8).

**Apply mechanics** (`Armor.cpp` / `Armor.hpp`):
- Applied inside RCST (`Armor::ApplyDefaultArmor`) — armor first, skills on top; two-phase Revert/Apply so re-runs (respec/respawn/skill-save) compose without stacking.
- **class 157** (Health-Mod 390 / HealthMax 412) → `ApplyBuff` (buff registry; folds into HEALTH_MAX 304 via the 412/390→304 recompute).
- **class 80** (protection 217/218/219) → **direct `prop.m_fRaw` write** (CalcProtection reads `m_fRaw` — §8), no buff-registry fan-out.
- Armor buffs register with **`nReqDeviceInstId = 0`** (global player stat, so `GetBuffedProperty` with devInst 0 picks them up), NOT the inventory id.
- No-op for bots / pawns with no equipped armor rows.

**Mitigation example** — full +21 Ranged armor vs Ballista OC (rating 100): Ranged axis 21/100 = 21%, combined
multiplicatively with base Physical 30% → `0.70 × 0.79 = 0.553` → **~44.7% mitigation** (vs 30% bare). Armor
protection is the second (attack-type) axis on top of the damage-type (Physical) axis.

**Full live mod pool (definitive — enumerated across ALL player armor in `gaa.db`):** exactly four rolled mods —
`24165` Ranged +0.5 (×32670), `24074` HealthMax +0.5% (×5460), `24083` AOE +0.5 (×1968), `24170` Melee +0.5
(×450) — plus the 7 per-slot Health-Mod +10% baselines. **No spread beyond these**: no higher-value variants,
no Physical/Fire/Energy (damage-type) protection, no CC protection.

**Axis-source separation (key):** armor contributes **attack-type protection (217/218/219) + Max Health ONLY**.
It gives **zero damage-type protection** (155/156/157). So in `CalcProtection`'s two multiplicative axes:
- **damage-type axis** (Physical 155 etc.) ← base attributes (+30) + skills only
- **attack-type axis** (Ranged 218 etc.) ← armor + a few tank skills (Heavy Armor, Super Tank, …) only

**Only open item needs a live test:** end-to-end confirmation that an equipped set's protection actually lands
on the pawn's 217/218/219 `m_fRaw` as predicted (the +70% HP half is already confirmed via base 1300 → 2210).

## 10. prop 316 "Additional Damage Taken" (vulnerability channel)

A multiplier that makes the **target take more** damage, applied **before** protection via
`TgEffectDamage.CheckDamageTakenModifier` (TgEffectDamage.uc:149; `ProtectionModifier` is :165).
Queries the target's own buff registry with `eRequestContext=2` (damage-taken context) for prop-316
entries (`TGPID_EFFECT_DAMAGE_TAKEN_MODIFIER = 316`), scoped by the effect's category/skill/devInst.

**Full damage order:**
```
D = base weapon damage
  → × attacker offensive buffs   (CheckEffectBuffModifier: 65/385/214…)          [offense, §1-6]
  → × (1 + Σ prop-316 on target) (CheckDamageTakenModifier)                        ← BEFORE protection
  → × Π(1 − protN/rating)        (ProtectionModifier: category×dmg-type×attack)    [defense, §8]
  → health-cap clamp (non-bot, §8)
```
A +30% vulnerability inflates the whole hit 30%, THEN protection mitigates the inflated value.

**Sources (verified in `gaa.db`) — attacker-applied, NOT a defender stat:**
- **Zero** come from player skill trees (155–163) — no defensive build produces it.
- **All** sources are **devices/weapons**: on-hit debuffs (type 264, situational 509, lifetime 0.5–8s) that mark
  the *target*. Two tiers: **class-157 vulnerability +10%…+30%** (meaningful; e.g. eg16908 +30%/0.5s, eg22297
  +30%/8s), and **class-80 tiny per-hit ~+0.2–0.7%** on specific weapons.

**Scope note:** out of scope for the defensive-build math (skills + armor) — it lives in the *attacker's* weapon
kit ("marked for death"). Documented for pipeline completeness; a defender can't stack or mitigate it via build.

**No skill / no poison gating (verified):** zero player-tree skills (155–163) produce prop 316 (full Medic Poison
tree dumped — none touch it), and **every** prop-316 effect has `required_category = 0` (none are poison-conditional
or otherwise gated). Canonical example: **Pain Gun I** (device 1802, +15% for 1s).

**"Damage taken while poisoned" — UC-confirmed NON-existent (2026-07-31):** checked the decompiled UC. `TgDamageTypePoison`
is cosmetic only (death/writhe anims, rest-speed). The damage pipeline (`TgEffectDamage.CheckDamageTakenModifier` /
`ProtectionModifier`) has **zero** references to poison or any condition. `TGCT_POISON` exists only as a condition
flag for `TgAIController.HasCondition` (AI/Kismet), never read by damage math. So poison is purely DoT of the poison
type; a poisoned target takes normal damage from everything else. No hook exists to make "poisoned → more damage."
(Note: this asm.dat capture has duplicate `effect_group_id` rows — bulk effect-group JOINs are unreliable; use
direct `WHERE effect_group_id=` dumps or the UC source.)

## 11. Offensive damage skill catalogue (player trees)

Every skill in the player trees (155–163) that contributes to damage. **Four distinct kinds** — they do NOT
all boost direct weapon fire:

- **Direct weapon-damage %** — prop 212 Melee / 214 Range / 321 AOE. Scales your actual hit, **gated by the
  weapon's attack type** (a ranged weapon only gains from Range 214; Melee/AOE nodes do nothing for it).
- **Effect Potency (376)** — amplifies **applied effect magnitudes** (poison DoT, debuffs, heals, buffs), **NOT
  direct weapon fire.** Confirmed by the §0 test: raw Ballista damage was identical with/without Eagle Eye
  (1600 both) — potency is a poison/debuff/support stat.
  - **⚠ prop 376 is rendered CONTEXTUALLY in tooltips, never as raw "potency"** — the game labels it by scope:
    Super Destroyer → "Movement Penalty", Eagle Eye → weapon-debuff amp, Death Medic → "Disease" (amplifies
    debuffs affecting a target's **Protection or Healing Received**). Don't infer a 376 node's meaning from the
    prop name — check the tooltip/scope.
  - **Anti-heal debuff channel:** prop **330 (Effect Healing Modifier)** reduced on a target = "less Healing
    Received"; amplified by Death Medic potency alongside protection-reduction debuffs.
- **Pet Damage (350)** — drone/robotic/pet builds only.
- **Buff-Damage (361)** — boosts the damage your *buffs* confer (Repair Overcharge).

| Tree | Skill (id) | Damage contribution |
|---|---|---|
| **Balanced** | Damage Increase Melee/Ranged/AoE (680/528/527) | +6% Melee / +6% Range / +6% AOE (each a separate node) |
| Balanced | Super Agent (677) | +5% Melee/Range/AOE |
| **Medic/Healer** | Buff Enhancement (903) | +40% Potency |
| **Medic/Poison** | Bio Rifle Damage (675) | +10% Range, +20% Potency |
| Medic/Poison | Death Medic (815) | **debuff-potency amplifier** — +50%/+200%/+200% Effect Potency for Medic Melee/Guns/Area-Poisons; tooltip: "increases potency of debuffs that affect a target's **Protection or Healing Received**" (rendered as "Disease"). Medic analog of Eagle Eye — amplifies the disease weapons' protection-reduction + heal-received (330) debuffs. NOT direct weapon damage. Also +60 self-heal per Combat-Offhand hit. |
| Medic/Poison | Fighting Medic (814) / Combat Off-Hand Power (807) / Quick Poisons (811) | +8% / +10% / +5% AOE |
| Medic/Poison | Medic Melee I/II/III (891/892/893) | +3/4/5% Melee |
| **Assault/Tank** | Assault Melee I/II/III (888/889/890) | +3/4/5% Melee |
| **Assault/Destroyer** | Assault Guns Damage (773) | +10% Range |
| Assault/Destroyer | Launcher Damage (710) | +10% AOE |
| Assault/Destroyer | Heavy Impact (910) | +200% Potency |
| Assault/Destroyer | Super Destroyer (682) | +6% Range/AOE dmg, +6% Pet dmg, +20% AOE/Pet radius, +20% Device Range, +4% Accuracy, **−100% Effect Potency** = in-game **"−100% Movement Penalty"** (CONFIRMED via tooltip: "move normally while firing minigun devices" — minigun movement penalty is a potency-scaled effect that −100% potency zeroes; NOT a damage stat) |
| Assault/Destroyer | Combat Off-Hand Power (807) | +10% AOE, +10% Pet |
| **Recon/Infiltration** | Explosives Damage (827) / Heavy Explosives (584) | +15% AOE / +10% AOE +25% Potency |
| Recon/Infiltration | Super Ninja (692) | +6% Melee, +10% AOE |
| Recon/Infiltration | Recon Melee I/II/III (894/895/896) | +3/4/5% Melee |
| **Recon/Marksman** | Recon Rifle Damage (598) | +10% Range |
| Recon/Marksman | Super Sharpshooter (693) | +10% Range passive **+ 5% Range on-hit (3s)** |
| Recon/Marksman | Sureshot (914) | +4% Range |
| Recon/Marksman | Eagle Eye (834) / Stim Boost (831) | +50% Potency / +50% Potency |
| **Robotic/Engineer** | Robotic Melee I/II/III (885/886/887) | +3/4/5% Melee |
| Robotic/Engineer | Repair Overcharge (613) | +10% Buff-Damage |
| Robotic/Engineer | Cyber Specialist (851) / Station Buff (795) | +50% Potency / +50% Potency |
| Robotic/Engineer | Super Engineer (714) / Heavy Artillery (845) | +5% Pet |
| **Robotic/Drones** | Robotics Rifle Damage (673) | +10% Range, +10% AOE |
| Robotic/Drones | Drone Damage (789) / Super Drones (797) | +10% / +5% Pet |
| Robotic/Drones | Super Supporter (798) | +5% Range/AOE, +10% Pet |

**Full Recon ranged direct-damage stack:** Balanced +6% + Super Agent +5% + Recon Rifle Damage +10% +
Super Sharpshooter +10% (+5% on-hit) + Sureshot +4% = **~+35% passive / +40% post-hit** to Range-Damage (214).
All register as class-157 buffs (type 261 passive; SSS on-hit is type 759) → applied at hit time via
`CheckEffectBuffModifier` (§1). Melee lines (+3/4/5%) exist in every class; potency-heavy trees (Medic Poison,
Assault Destroyer, Robotic) are effect/pet builds, not direct-fire.

## 12. Death Medic interactions (debuff-potency amplifier)

Death Medic (815) potency is scoped by `required_skill_id` to three weapon skills: **Medic Melee (370) +50%**,
**Medic Guns (405) +200%**, **Area Poisons (336) +200%**. It amplifies debuffs those weapons apply that reduce a
target's **Protection** (Physical 155) or **Healing Received** (prop **210** Effect Heal Modifier (Self)) — same
architecture as Eagle Eye → Ballista, medic version.

**Interacting weapons (player-facing "base" items carrying the real weapon skill):**
| Skill (Death Medic tier) | Weapons |
|---|---|
| **Area Poisons (336, +200%)** | Poison Grenade, **PowerVirus Grenade**, Poison Aura, Neutralize Wave, (Backup) Neutralize Wave |
| **Medic Melee (370, +50%)** | Poison Injector, Life Stealer |
| **Medic Guns (405, +200%)** | Agonizer |

Debuffs applied (base, pre-amplification) come from the spawned deployable/cloud: **Physical protection −2…−5**
and **Self-Heal (prop 210) −5%…−20%** — matching the tooltip's "Protection or Healing Received". Amplification
scope = the **device's** skill at apply time (OriginResolver), not the effect group's `required_skill_id`.

**NOT in Death Medic scope:** **Venom Bomb** (skill **328**, a different skill), the **Toxin** family (skill 1 /
NPC-flagged), and **Thid's Grenade** (an AVA mode-specific offhand).

**DB caveats (this asm.dat capture):** (1) each weapon family has tier items I/II/III at `skill=1` plus one base
item with the real skill (336/370/405) — filter on the base item. (2) `WHERE skill_id=` is unreliable (duplicate
rows return different values per query path) — name-based lookup was needed. (3) item→device/deployable links are
absent (`ref_device_id`/`ref_deployable_id`=0 on these grenades), so the item↔debuff mapping can't be done purely
in SQL — the roster above is by skill scope; per-weapon debuff values come from the deployable side.

## 13. Weapon deep-dive: Neutralize Wave (buff-strip)

Medic **Area Poisons (336)** weapon. Item 3642 (main), 6517 ("(Backup)"). Physical damage (type 113), Range
attack (85), rating 100. Tooltip: *"Removes shields and buffs. Removes all positive healing effects. Deals
damage for each effect removed."*

**Mechanic:** ~14 effect groups, each = **`Remove Effect` (prop 140)** + **`Health −110`** (one −115). Prop-140
resolves in `TgEffect.uc:301` to `RemoveEffectGroupsByCategory(m_nPropertyValueId, 99)` — strips ALL groups of a
target category. The groups cover **15 categories** including **770 = PERSONAL_SHIELD** (770–775, 607, 886,
935/936, 985, …) — shields, buffs, HoTs.

**Damage scaling** = "110 per effect category removed" + base:
- Unbuffed target: ~110–115 Physical.
- Buffed/shielded target: 110 × (categories stripped) + base — punishes buff-stacking hard.
- Each 110 chunk is Physical+Range → mitigated by target's Physical (155) + Ranged (218) protection (§8).

**Synergy:** strips shield + buff-sourced protection FIRST, so its own damage (and team follow-up) lands into
reduced mitigation. The dedicated anti-tank / anti-support / anti-heal tool.

**Death Medic:** skill 336 → in the +200% Area-Poisons potency scope. Main (3642) is strip+damage only (no
protection/heal debuff; potency doesn't scale direct damage, so its core isn't boosted). "(Backup)" (6517) ALSO
applies Physical −2 / Self-Heal −15% (7 s) — that debuff IS amplified by Death Medic.

## 14. Complete skill→device interaction matrix (data-driven)

All 117 tree skills (groups 155–163) resolved to inventory devices **mechanically**, not hand-curated —
rendered in `docs/claude/theorycraft-console/device-console.html`. Resolution rules (proven from code):

1. **`required_skill_id` gating** → devices whose item `skill_id` matches (buff-registry match rule §0).
   Covers all weapon-line skills (Recon Rifle */Assault Guns */Bio Rifle */Launcher */Robotics Rifle *…),
   offhand lines (Stims/Explosives/Grenades/Shields/Overdrives/Heals/Stations/Turrets/Drones/Repair),
   potency amplifiers (Eagle Eye→327, Death Medic→336/370/405, Buff Enhancement→250/252, Heavy Impact→
   276/301/333/351/898, Stim Boost→330, Station Buff/Cyber Specialist→283), and **Jetpack Power→358–361**.
2. **`required_category` gating** (reactive, type 1104): Aegis Armament→cat 770 (shield devices),
   Stealth Protection→cat 621 (stealth devices).
3. **Situational gates** (type 505): Killer Instinct (HP>75%), Group Heal Savior (HP<25%), Combat
   Off-Hand Utility (HP>75%). Type 759/264 = on-hit (Super Sharpshooter, Super Healer +50 group-heal,
   Death Medic +60 self-heal, Super Engineer +5 Phys on repair-hit).
4. **Unscoped passives** resolve by prop semantics + class compatibility (212→melee, 214→ranged dmg
   devices, 321→AOE, 350/366/381-383→pets, 330→healers, 357/337→boosts).

**Notable data-driven finds:** Super Tank = **GroundSpeed −100% scoped to Shields (909)** — removes the
shield movement penalty (same pattern as Super Destroyer's −100% potency scoped to Assault Guns 276 =
minigun movement penalty). Zoom effects live in **type-266 Aim effect groups** (e.g. Raven/Rockwind
`+30% Range/EffRange, −20% spread while zoomed` — NOT in the mode stat block; earlier "no scoped range" claim
was wrong). Zero unresolved skill gates; 128/128 inventory devices have interactions.

Generators (scratchpad, session-local): `gen_ix.py` (resolver) → `ix.json`; `gen2.py` (inventory model);
`gen3.py` (console renderer). Inventory = `ga_players_inventory` user 2381 ("used" = present in inventory).

## Stale references caught (fix opportunistically)
- `docs/claude/effect-system.md` §"CloneEffectGroup pre-scaling" — says pre-scale `m_fBase` in CloneEffectGroup; the code now scales in `CheckEffectBuffModifier` instead.
- `TgEffect__CheckEffectBuffModifier.cpp` header cites `.planning/effect-buff-property-canonical.md` — file no longer exists.
- `Armor.hpp` header says each ApplyBuff entry is tagged with the armor's `ga_players_inventory.id` as `nReqDeviceInstId`; the code (`Armor.cpp`) actually passes `devInst = 0` (global player stat) — the .hpp comment is stale.


## 15. Resolver rules (derived while building the device test bench, 2026-08-01)

The console's `GA.resolve` takes a device + an allocated skill set and returns modified
numbers. Two rules had to be established to make it agree with the measured Ballista data;
both are general and apply to the eventual full-build simulator.

### 15.1 Modifier properties vs. effect properties — never compose them

A skill property is one of two kinds, and conflating them produces wrong numbers:

| Kind | Meaning | Examples |
|---|---|---|
| **Modifier** | scales an existing device number | 65 Dmg, 212/214/321/350 attack-type, 330 Healing, 376 Potency, 208 Lifetime, 232 Attack Rate, 242 Power Cost, 203 Recharge, 114/207 Range, 355 Pet Lifespan, 366 Pet Max HP |
| **Effect** | applies its own independent effect | 51/211 Health, 155-371 protections, CC (166 Stun, 338 Root, 305 Taunt, …), 316 Dmg Taken |

Concretely: **Killer Instinct's "-10 protection" does NOT modify the Ballista's own
"-10 protection" debuff.** They are two separate debuffs landing on the same target. An
identity-property match (skill prop 155 vs. device chip prop 155) is therefore *not*
grounds to compose them — the resolver deliberately has no identity fallback, and an
unlisted property simply has no known modifier.

This matches the measured A/B result in §0: Ballista -10 amplified to -13 by Eagle Eye,
**plus** Killer Instinct's separate -10 — not a single combined -20 or -13-of-20.

### 15.2 `property_value_id` is a category scope on ANY property, not just Potency

Every skill effect carrying a non-zero `property_value_id` is scoped to that effect
**category** and may not touch any other. Confirmed set from `gaa.db`:

| Prop | Scoped instances |
|---|---|
| 376 Effect Potency | 305 Disease, 769 General Debuff, 773 Stim Boost, 775 General Buff, 875 Knockback, 935 Proximity Dmg Buff, 936 Personal Dmg Buff, 986 Additional Damage, 1283 Power Pool Buff, 1324/1326/1327 Station |
| 208 Effect Lifetime | 303 Poison, 304 Slow, 378 Stun, 607 Ability, 719 Ignite, 769 General Debuff, 774 Stim Resistance, 986 Additional Damage, 1601 Threat |
| 66 Effect GroundSpeed | 1452 Shield Movement Penalty |

So the scope check is `if (fpv && fpv !== effectGroupCategory) skip` — generic, not a
376 special case. This is what makes Eagle Eye reach the Ballista's protection debuff
(category **986**) while being unable to reach Killer Instinct's (category **302**).

**Verified against the measured data:** with the user's real build, the bench reproduces
Ballista Phys Prot **-10 -> -13** and lifetime **5s -> 6s** (Eagle Eye +30% potency and
+20% lifetime, both scoped to 986), and -10 / 5s with Eagle Eye removed.

### 15.3 Known limits of the current resolver

- **Single layer only.** Skill percentages sum within the skill layer. The item/armour
  layer (which *multiplies* with it, per §6 and the 1300 x 1.70 x 1.25 = 2762 check) is
  not yet applied to device numbers.
- **No mitigation stage.** Output is the attacker-side value; `CalcProtection` (§8) and
  the per-hit health cap are not run. That is the next piece for a two-sided simulator.
- **Refire/attack-rate is treated as a linear inverse** (+5% attack rate -> -5% refire).
  Not verified against the engine.
- Effect-group types 261/283 are treated as always-on; 264/272/505/759/1104 as
  situational and gated behind a toggle.



## 16. Device stat identities corrected (AOE Shield, 2026-08-01)

In-game AOE Shield tooltip:

```
AOE Shield [sssccc]
Cooldown     60
Durability   100/100
Immune to AOE damage for 10s or 2000 damage.
Speed reduced.
+70% Output Mod   +12% Shield Health   -6% Cooldown
```

### 16.1 Cooldown is prop 4, not 203

**Prop 4 "Recharge Time"** is the device stat (701 device-mode rows). **Prop 203 "Recharge Time
Modifier"** is the modifier and appears on **zero** device rows. The console had 203 in its
device-stat map, so cooldown never rendered for any device.

### 16.2 A shield's absorb pool is `asm_data_set_effect_groups.health`

The pool is **not** in `asm_data_set_effects` at all -- it is the `health` column on the effect
GROUP. AOE Shield: `health = 2000`, matching the tooltip's "or 2000 damage". This is what
**386 Effect Shield Modifier** ("+12% Shield Health") scales.

Inventory devices carrying a pool:

| Device | health | category |
|---|---|---|
| AOE Shield | 2000 | 770 Personal Shield |
| Range Shield | 2000 | 770 Personal Shield |
| Protection Boost | 2000 | 302 |

The same column means something else elsewhere and must not be read as a shield pool:
category **304 Slow** carries `health = 1` (on Mace and Shield, Heavy Wrench, EnergyBurn Mace)
and category **877 Remove Effect** carries 40/80.

> **"Durability 100/100" in the tooltip is the retired gear-durability mechanic** -- gear lost a
> point of durability per death and had to be repaired. It is unrelated to shields and does not
> appear in this data. An earlier revision of this section wrongly identified the shield pool as
> durability; that was wrong and is corrected here.

The device's `Protection - AOE 100` (prop 219, category 770) is a genuine protection value and is
separate from the pool.

### 16.3 Verified two-layer arithmetic

Rolled mods are the ITEM layer, skills the SKILL layer, and they **multiply**:

| Value | Base | Item (`sssccc`) | Skills (full Tank) | Result |
|---|---|---|---|---|
| Shield Health | 2000 | +12% | +25% Shield Strength | 2000 x 1.12 x 1.25 = **2800** |
| Duration | 10s | -- | +40% Shield Strength | 10 x 1.40 = **14s** |
| Cooldown | 60s | -6% | -15% Shield Readiness | 60 x 0.94 x 0.85 = **47.94s** |
| Speed penalty | -10% | -- | Super Tank -100% (scoped to cat 1452) | **removed** |
| AOE Prot | 100 | -- | -- | **100** (untouched by shield mods) |

Super Tank's prop-66 entry is scoped to category **1452 "Shield Movement Penalty"** -- it does not
remove slows generally, only the shield's own movement penalty.

### 16.4 Display polarity

Direction of benefit is not derivable from sign alone. Lower is better for props 4 (recharge),
53 (refire), 279 (deploy), 242/322 (power), and for any `debuff`-kind chip landing on its own user
(`Self:` prefix, or category 1452). A duration inherits its chip's polarity: longer is better for a
buff, worse for a self-inflicted penalty.

### 16.5 Still unverified

Whether **Output Mod is re-applied at hit time** for damage (on top of the displayed tooltip
number) remains open from S6. The bench counts it once, in the item layer, and says so.


## 17. The ACTIVE layer (2026-08-01)

The bench's on-hit/conditional toggle was generalised into an **"is this device active?"** flag,
because they are the same question: a shield being up, a weapon landing a hit, and a boost running
are all "the trigger condition currently holds". This is the first two-sided piece of the
simulator -- the same shape will later express "this medic used Protection Wave on you".

### 17.1 Conditional skill effects must not sit in the passive total

`asm` skill effects are one of four kinds: **passive** (255 of them), **conditional** (6),
**on-hit** (5), **reactive** (2). Only `passive` is always-on once allocated.

Aegis Armament (reactive, `required_category_value_id` = **770 Personal Shield**) grants
`Protection - Physical +25` **only while a Personal Shield effect is up**. Counting it in the
character sheet unconditionally is wrong, and counting it *both* there and in the active layer
double-counts it (observed: Physical showed 80 = 30 base + 25 + 25).

Rule applied: non-passive skill effects are **excluded from the passive total** and drawn struck
through as `reactive - not active`. When their trigger goes live the ACTIVE layer supplies them,
and the dormant duplicate is suppressed.

| Physical Protection | Inactive | Shield up |
|---|---|---|
| Human Base Attributes | +30 | +30 |
| Aegis Armament (reactive) | ~~+25~~ dormant | **+25** active |
| **Total** | **30** | **55** |

### 17.2 What an active device contributes

1. **Skills gated on it** -- reactive/conditional entries whose category the device produces.
2. **The device's own buffs on its user.** A *positive* protection is a buff you gain; a
   *negative* one is a debuff you inflict on what you hit, so only positives are taken.
3. **Shield pools**, paired to the damage types they cover by shared effect category.

### 17.3 Absorption is shown as terms, not a number

A protection covered by an active shield pool gets an `absorbed` badge and the shield's actual
terms rather than a bare "immune":

> **SHIELD · AOE Shield** -- absorbs **2800** damage of this type, or **14s** -- whichever runs out first

Both limbs matter: the tooltip's "Immune to AOE damage for 10s **or** 2000 damage" is a duration
*and* a damage cap, and both are modified (2000 -> 2800 by mods+Shield Strength, 10s -> 14s by
Shield Strength's lifetime). Presenting it as a flat "IMMUNE" would hide the cap that actually
decides the fight.


## 18. Protection is flat, never a percentage (Aegis Armament, 2026-08-01)

Raised because Aegis Armament's in-game text reads *"you gain an additional 25% Physical
resistance"* while the data stores `25.0`.

**The data is unambiguous.** Aegis uses `calc_method_value_id = 67`, which the game's own enum
(`asm_data_set_valid_values`) names **"Add (+)"**. The percentage methods exist -- 68 "Increase (+%)",
69 "Decrease (-%)" -- and Aegis does not use them. Across **all 42** skill effects on the protection
properties (155/217/218/219) in every tree, every one uses Add or Subtract. **Not one uses a
percentage method.** Protection is never stored as a percentage in this game.

**The tooltips are inconsistent prose, not mechanics.** Only 4 protection skills phrase it as a
percentage (Aegis Armament, Killer Instinct, Group Heal Savior, Combat Off-Hand Utility). Most read
like Built Tough -- *"Provides AOE Protection"* -- for a flat +15. Killer Instinct's stored string
says *"lower his protections by 10"* with no percent sign, while its in-game display shows "10%".

**Why the percentage reading is nearly right.** From `CalcProtection` (S8):
`fPercProtection = nProtection / nDeviceRating`. At the reference attack rating of **100** -- what
both OC weapons carry -- a flat 25 is exactly 25% reduction. The two coincide numerically at
rating 100, which is presumably why the text was written that way. Independently confirmed by the
measured Ballista test in S8: Physical 30 at rating 100 gave `1600 x (1 - 0.30) = 1120`.

**Where the distinction bites:**
- **Rating penetrates.** +25 protection is 12.5% against a rating-200 weapon, not 25%.
- **It adds, it does not scale.** Base 30 + Aegis 25 = **55**, not 30 x 1.25 = 37.5.
- **Axes multiply.** 55 Physical alongside a Ranged axis is `(1 - 0.55) x (1 - X)`.
- **At or above the attack rating it is total immunity.** This is the mechanism behind AOE
  Shield's "Immune to AOE damage": its flat **+100** on a base of 24 gives 124 vs rating 100,
  i.e. mitigation >= 100%. The 2000-point pool is the cap on how much that immunity absorbs.

The console now prints the mitigation next to every protection stat (`55` -> `55%`), shows
`immune` once protection reaches the attack rating, and footnotes the rating caveat.

### Side note -- where the tooltip strings live

`asm_data_set_msg_translations (msg_id, message, sound_res_id)`. Aegis: name **65364**,
description **65365**. Note the strings are **duplicated**: `asm_data_set_skill_group_skills`
carries denormalised `name_msg_translated` / `desc_msg_translated` copies, so editing one place
leaves the other stale.

Whether an edit reaches the client is **not yet established**. These tables are populated by
`src/Database/AsmDataCapture/AsmDataCapture.cpp`, which *captures* rows by hooking the `CMarshal`
getters and the `CAm*::Load*Marshal` loaders (`AsmDataCapture::bPopulateDatabase` in `dllmain.cpp`)
-- that is a read path. Before assuming tooltips are editable, confirm whether the server *sends*
the ASM data set to clients over the same marshal path or whether each client loads it locally.
Start at `CMarshal__GetArray` and the `CAm*__Load*Marshal` hooks.


## 19. Character import - real saved builds (2026-08-01)

The console can now load a player's actual saved build rather than a synthesised one.

### 19.1 Schema

| Table | Key columns | Notes |
|---|---|---|
| `ga_users` | `id` (**not** `user_id`), `username` | join target |
| `ga_characters` | `id`, `user_id`, `profile_id` = CLASS, `current_item_profile_id`, `deleted_at` | one row per character |
| `ga_character_devices` | `character_id`, `item_profile_id` 1-5, `inventory_id`, `equipped_slot` | |
| `ga_character_skills` | `character_id`, `item_profile_id`, `skill_group_id`, `skill_id`, `points` | |

Two different "profile" ids share the name and must not be confused:
`ga_characters.profile_id` is the **class** (680 Assault / 567 Medic / 681 Recon / 679 Robotics);
`item_profile_id` is the **loadout slot 1-5**. Each of the 5 profiles carries its own gear set,
armour set *and* skill build - switching profile in game switches all three.

`equipped_slot` doubles as the slot type: 1-23 are devices and cosmetics, **1130-1143 are armour
pieces** (1130 Head, 1143 Shoulder, 1133 Chest, 1136 Arm, 1132 Hand, 1139 Leg, 1142 Foot).
Cosmetic-only slots (996-1001 dyes/trails, 202 suit) carry no mechanics and are filtered out.

### 19.2 Mods come from the equipped INSTANCE

`ga_character_devices.inventory_id` -> `ga_players_inventory.id` gives that specific item's
`mod_effect_group_ids`. This is the real roll on the equipped piece, not a representative variant
from the device pool - so the bench can be driven with exactly what the character is carrying.

Jeronix (user 2381) has 4 built characters, each with 5 fully-populated profiles
(9 devices + 7 armour + 13 skills).

### 19.3 Verified against the real Recon build (char 74, profile 2)

Armour, all 7 pieces rolled `rrrnnn`, decoded per piece as
`+10% Health Mod (390)` base, `3 x +0.5 Protection-Ranged (218)`, `3 x +0.5% Health Max (412)`:

```
7 pieces -> Health Mod +70%  +  Health Max Modifier +10.5%  =  +80.5% item layer
            Protection - Ranged +10.5
HP = 1300 x 1.805 (armour) x 1.10 (skills) = 2581.15 -> 2581   [game truncates]
```

Matches the console's headline exactly. Profile 2 (explosives/stealth, HP 2581) and profile 4
(melee, HP 2210) load completely different skills, gear and armour, confirming the per-profile
split is real.

Benching an equipped Raven SMG OC uses its own `dddddd` roll:
`75 x (1 + 0.75 Output + 0.12 Range-Dmg + 0.09 Damage) = 147`.


## 20. Effect stacking / mutual exclusion (2026-08-01)

**`asm_data_set_effect_groups.application_value_id` is the stacking rule, scoped by
`category_value_id`.** Two effect groups in the same category do not both run unless the rule
is Stackable.

| id | rule | count |
|---|---|---|
| 155 | Stackable | most |
| 156 | Newest Wins | |
| 157 | Strongest Wins | uses `application_value` as the strength |
| 836 | Refresh | re-arms the duration |
| 874 | Oldest Wins | |

Confirmed against both reported cases:
- **AOE Shield / Range Shield** - both category **770 Personal Shield**, **Newest Wins**. They
  cannot both be up; the newer replaces the older. (They also collide in category **1452 Shield
  Movement Penalty**, so a naive implementation prints the conflict twice.)
- **Healing Grenade / the Nanite guns** - category **772 Regeneration**, **Strongest Wins**, with
  `application_value` carrying the HoT magnitude (Healing Grenade 64). Members in this inventory:
  Adrenaline Gun, Healing Grenade, Nanite Enhancement System, Nanite Repair, Nanite Restoration
  System - and HUMAN BASE ATTRIBUTES, which is why the Rest regen is not additive with a HoT.
  Note the Medic device literally named **Regeneration** is category **1341**, *not* 772, so it
  does stack with a Healing Grenade.

Twenty such groups exist across this inventory, including Poison (303, Strongest), Disease (305,
Refresh), Knockback (875, Newest), Movement Penalty (1360, Strongest), Additional Damage (986,
Strongest - the Ballista debuff family) and Personal Damage Buff (936, Strongest).

### 20.1 Offensive effects never apply to their carrier

A device's negative effects are aimed at whatever it hits, never at the player holding it - a
Poison Aura cannot poison its owner. The console applies this as a hard guard: a negative chip
contributes to the player only when the game explicitly scopes it to self, i.e. a `Self:`
effect-group type (261/262/263/265/266/283/759/1104) or a self-penalty category such as **1452
Shield Movement Penalty** (which Super Tank's category-scoped prop-66 entry removes).

### 20.2 HUMAN BASE ATTRIBUTES is not an equippable

Device **864** is on every character and is already folded into the base player stats, so it is
excluded from the equipped list rather than shown as a toggleable item.


## 21. Output Mod is its OWN layer — the offensive chain, settled (2026-08-01)

Resolves the §6 open question. A Ballista OC `[dddddd]` fired with **no skills allocated** at a
target carrying only Human Base Attributes:

```
You hit YeXiuu for 867 damage (372 mitigated).
You hit YeXiuu for 991 damage (248 mitigated).
```

Both shots have the **same raw damage** — `867+372 = 1239` and `991+248 = 1239`. The difference is
mitigation only: 372/1239 = **0.300** (Physical 30 from HBA) and 248/1239 = **0.200** (30 − 10,
the Ballista's own protection debuff landing for the second shot). That independently re-confirms
`CalcProtection` at rating 100.

### The model

Item mods do **not** all sum. Output Mod (385) forms its own multiplicative layer:

```
damage = base
       × (1 + OutputMod)              // 385, on its own
       × (1 + Σ other item mods)      // 214 Range Dmg, 65 Damage, ...
       × (1 + Σ skill mods)
```

| candidate | raw | shot 1 | shot 2 |
|---|---|---|---|
| all item mods summed — `585 × 1.96` | 1146.60 | 802.6 | 917.3 |
| Output applied twice — `585 × 1.75 × 1.96` | 2006.55 | 1404.6 | 1605.2 |
| every mod multiplies — `585 × 1.75 × 1.12 × 1.09` | 1249.79 | 874.9 | 999.8 |
| **Output its own layer — `585 × 1.75 × 1.21`** | **1238.74** | **867.1** | **991.0** |
| **measured** | **1239** | **867** | **991** |

Matches to the unit on both shots.

**Consequences:**
- Output Mod is applied **once**, and the displayed tooltip (`585 × 1.75 = 1023`) is that layer
  alone — the remaining item mods are applied on top of it at hit time, not folded into it.
- The other rolled mods **sum with each other** (12 + 9 = 21%), they do not each multiply. The
  all-multiplying variant overshoots by 11.
- The console previously summed Output with the rest, understating every damage figure by ~8%.
  Fixed in `GA.resolve`'s `apply()`.

### Residual

The earlier skilled test (measured 1120 dealt) now models as 1536.03 raw → **1075**. Adding
Super Sharpshooter's on-hit +5% stack gives 1598 → **1118.6**, effectively the measured 1120 —
consistent, but it means that shot already carried the on-hit stack. Worth a clean re-test with
the skill build if an exact figure is wanted; the no-skill case above is unambiguous.
