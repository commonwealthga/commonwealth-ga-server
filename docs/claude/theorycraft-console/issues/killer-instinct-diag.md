# Killer Instinct self-shot leak — FIXED

**Status: fixed and verified in game, 2026-08-02.** Server-side, on branch
`killer-instinct-self-shot`. This file was a fix handoff; it is now the record of what the bug
actually was, because the handoff got the mechanism wrong in an instructive way.

**Companion:** [killer-instinct-self-shot-leak.md](killer-instinct-self-shot-leak.md) is the
player-facing report.

---

## 1. The bug

**Killer Instinct** is a Recon skill: hit a target above 75% HP with a rifle and their Physical
protection drops 10 for 3 seconds. It is an *on-hit* debuff, so it should only affect **subsequent**
shots — the shot that triggers it must be mitigated against the target's full protection.

It didn't. The **entire −10** reached the triggering shot. The Ballista's own on-hit debuff does
not do this; the two differ only in dispatch type — Killer Instinct is **type 505
(Hit-Situational)**, the Ballista debuff is **type 264 (Hit)**.

---

## 2. Why it looked partial — the correction that mattered

The original analysis (§6 of the old version of this file, now deleted) had this as a *partial*
leak of about 3.1 protection points, and spent its effort explaining a fraction that was never
there. The measurement it was reasoning from:

| Build | Shot 1 dealt | Shot 1 mitigated | apparent protection |
|---|---|---|---|
| Killer Instinct slotted | 1170 | 430 | 26.875 — unreachable |
| Killer Instinct removed | 1120 | 480 | 30 (clean) |

The reasoning was sound as far as it went: `CalcProtection` integer-floors protection, so 26.875
is not producible, and a full ordering leak should have read 1280 at protection 20. From those two
facts it concluded the shot's damage must be arriving in more than one piece, with the debuff
landing between them.

**There are no pieces. 1170 is not a mitigation figure at all.** It is the anti-one-shot health
cap in `TgEffectDamage.uc:174-200`:

```
nHealthCapEnd = FCeil(r_nHealthMaximum * 10/100);
nAmtToCap     = Health - nHealthCapEnd;
if(fProratedAmount > nAmtToCap) fProratedAmount = nAmtToCap;
```

On a bare 1300 HP target that is `1300 − 130 = 1170` exactly. It arms in `ApplyHit:1287`:

```
if((TargetPawn.Health / TargetPawn.r_nHealthMaximum) > (90 / 100))
    TargetPawn.s_bApplyHealthCap = true;
```

Both operands are ints, so `90/100` folds to 0 and `Health / r_nHealthMaximum` is 0 unless the
target is at *exactly* full health, where it is 1. The rig specified full HP before shot 1 of every
run, so it armed on every shot 1 and never on shot 2. It also inflates the reported mitigation,
because `SendDamageMessages` reports `pre − post` **after** the clamp — hence 430 rather than 320.

So the real shot-1 sequence with Killer Instinct slotted was `1600 × 0.80 = 1280`, clamped to 1170.
The full −10 was reaching the shot the whole time.

**The lesson worth keeping:** the value being unreachable was a real signal, correctly spotted. The
error was assuming the number reaching the log had only passed through mitigation. Two clamps sit
between `ProtectionModifier` and `SendDamageMessages`, and one of them only arms at exactly full
HP — which is precisely the condition a controlled first-shot test guarantees.

It also invalidated this file's proposed discriminating experiment. Effect group 26474 (a type-505
prop-155 debuff at −5 with the same gate) would have produced `1600 × 0.85 = 1360`, also above the
cap, and would have read 1170 as well — confirming the "partial leak" story with a second fake data
point.

---

## 3. Still settled — do not re-derive, and do not "fix"

Correct behaviour that survived the fix and was re-verified after it:

- **Eagle Eye's potency amplifies the Ballista's type-264 device debuff (−10 → −13) but does
  NOT amplify Killer Instinct.** Confirmed intended by the project owner. It is emergent, not
  special-cased: Eagle Eye's prop-376 potency registers in `m_EffectBuffInfo` scoped to
  `nReqSkillId = 327` (Recon Rifles); the device debuff carries skill 327 as its origin and
  matches, while Killer Instinct is skill-sourced so the potency query finds nothing.
  **Leave this alone.** The fix deliberately keys on nothing that would disturb it — see §6.
- **The HP gate re-evaluating per shot is intended.** Shot 1 applies Killer Instinct (target at
  100%); on shot 2 the target is below 75% so it does not *re-*apply, but shot 1's instance is
  still live for its 3s. That is the model working.
- **Mitigation % = Physical protection value, 1:1** at attack rating 100.

---

## 4. Identities

All verified against `gaa.db`.

| Thing | Value |
|---|---|
| Killer Instinct skill_id | **836** |
| Killer Instinct effect group | **16596** |
| — type | **505** Hit-Situational |
| — situational | **1271** `value = 75.0` → target HP **above** 75% |
| — effect | class **80** (direct-property), prop **155** Protection-Physical, base **10.0**, calc **70** (SUB) |
| — lifetime / category / application | 3.0s / 302 / 836 (Refresh) |
| Ballista OC | device **2110**, mode 2618 — damage eg 6164 (prop 51 −585), debuff eg **18975** type **264**, prop 155 −10, 5s, category 986 |
| Scorpia OC (the clean control) | device **3249**, mode 3472 — damage eg 9600 (prop 51 −585, identical), debuff eg **9603** type 264 but **prop 210** self-heal −40%, category 769 |
| Eagle Eye (do not disturb) | skill 834, eg 17244, type 261 Equip, prop 376 potency |
| Player base max HP (no armour) | **1300** → health cap floor 130, so cap = 1170 |

Enum decode: effect_group_type **264** = Hit · **505** = Hit-Situational · **759** = skill
self-buff · **261** = Equip. situational_type **509** = generic on ranged hit · **1271** = target
HP above `situational_value`% · **1270** = below.

**Scorpia OC is the better test weapon.** Same base damage (585), damage type (113), attack type
(177), attack rating (100), projectile (88) and skill gate (item skill_id 327) as the Ballista, but
its on-hit debuff is prop 210 rather than prop 155. Prop 210 is in none of `ProtectionModifier`'s
switches, so it cannot touch mitigation — which makes Eagle Eye inert and leaves Killer Instinct as
the only variable on shot 2.

---

## 5. Where the code is

All UnrealScript, none of it in the exe. Read from **github.com/commonwealthga/ga-source**
(UC under `unrealscript/`, Ghidra C++ of the exe under `decompiled/`).

`TgDeviceFire.ApplyHit` dispatch order, which is the whole bug:

```
1404   SubmitHitEffects(.., 0, 272)                  HIT_IN_AIR
1409   SubmitHitEffects(.., 0, 505, 1270)            \
1410   SubmitHitEffects(.., 0, 505, 1271)             |  HP-gated situational
1411   SubmitHitEffects(.., 1, 505, 1270)             |  ← Killer Instinct lands here
1412   SubmitHitEffects(.., 1, 505, 1271)            /
1445   SubmitHitEffects(.., 0, 264)                  ← device: base damage, THEN the device debuff
1480   SubmitHitEffects(.., 0, 505, 509)             backstab
1491   SubmitHitEffects(.., 1, 505, melee)           situational melee
1495   SubmitHitEffects(.., 1, 264)                  skill
```

`SubmitEffect → ProcessEffect → ApplyEffects → ApplyToProperty` is fully synchronous, so the
debuff is in `m_fRaw` before `ProtectionModifier` reads it.

Note that **every other 505 dispatch sits after the damage submit** — only the 1270/1271 block is
ahead of it. That asymmetry is what makes this read as an authoring slip rather than a design.
It is also why the Ballista's own debuff was always clean: eg 18975 rides the same 264 pass as
eg 6164 and comes after it in the fire mode's list.

---

## 6. The fix

`src/GameServer/TgGame/_effect_core/HitSituationalMitigation.{hpp,cpp}` on branch
`killer-instinct-self-shot`, wired into two hooks we already owned. The apply order is UC, so it
is not ours to change. Instead the debuff applies exactly as before — gate, potency query,
lifetime, HUD slot and `Remove` path all canonical — and the target is lent its pre-debuff
`m_fRaw` back for the duration of that impact's mitigation only:

| call | hook | UC site |
|---|---|---|
| `NoteDebuffApplied` | `TgEffect::CheckEffectBuffModifier` on a 505/1270/1271 protection effect | `TgEffect.uc:115` — before `ApplyToProperty` writes it |
| `BeginImpactMitigation` | same hook, on a `TgEffectDamage` effect | `TgEffectDamage.uc:131` — before `ProtectionModifier` |
| `EndImpactMitigation` | `TgEffectManager::RemoveEffectGroupsByCategory(431, 99)` | `TgEffectDamage.uc:206` — after mitigation and the cap, before `TakeDamage` |

Both brackets sit inside one UC function with no `return` between them, and `TgEffectDamage.uc:202`
gates the closing call on a pawn target — which is why the guard only ever arms for pawn victims.
The swap cannot outlive the damage application that opened it. It stores the *delta* rather than an
absolute, so anything else writing that property inside the window still composes.

Kept narrow on purpose:

- only the props `ProtectionModifier` can actually consume are eligible (the three switches in
  `TgEffectGroup.uc`);
- the guard never assumes −10 — it captures the pre-value and re-applies whatever delta it
  measured, so a potency-amplified 505 debuff would be handled correctly;
- nothing is keyed on `m_nSourceDeviceInstId`, which is what scopes Eagle Eye's potency to weapon
  debuffs (§3) and must not be disturbed;
- category-963 damage groups and DOT interval ticks after the first skip the opening bracket at
  `TgEffectDamage.uc:128/103` and keep their previous behaviour. Neither is a triggering shot.

---

## 7. Verification (2026-08-02)

Scorpia OC, victim in +health armour with no protection mods (so base Physical protection 30),
point-blank, full HP before shot 1. Second run drops the Killer Instinct point rather than
reallocating it, so offence is provably constant — confirmed by identical shot-1 raws.

| Run | Shot | raw | protection | predicted | measured |
|---|---|---|---|---|---|
| KI (9 pts) | 1 | 1536 | **30** | 1536 × 0.70 = 1075.2 → 1075 / 461 | **1075 / 461** |
| KI | 2, 3 | 1598 | **20** | 1598 × 0.80 = 1278.4 → 1278 / 320 | **1278 / 320** |
| no KI (8 pts) | 1 | 1536 | **30** | 1075 / 461 | **1075 / 461** |
| no KI | 2, 3 | 1598 | **30** | 1598 × 0.70 = 1118.6 → 1119 / 479 | **1119 / 479** |

All six rows to the unit. Shot 1 identical across builds; the −10 still lands in full on shots
2–3; Killer Instinct reads −10 rather than −13 with Eagle Eye slotted in **both** runs, so the §3
asymmetry survives. A Ballista run confirms the other half — its device debuff still amplified to
−13 (shot 2 at protection 17 without KI, protection 7 with).

---

## 8. Confirmed from source, so do not re-derive

- `TgEffectGroup.CalcAttackTypeProtection` is a **switch** — exactly one attack-type axis ever
  applies (3 → 219 AOE, 1 → 217 Melee, 2 → 218 Range). They never stack.
- `TgEffectDamage.ProtectionModifier` order: Hunter → Category → DamageType → AttackType, the last
  only for `m_nCategoryCode` 302 or 963.
- `m_eAttackType` is **never assigned in UnrealScript** — populated natively. Empirically a ranged
  attack with a splash radius (prop 6) resolves as AOE; the Ballista and Scorpia have no prop 6 and
  resolve as Range, with base ranged protection 0 on a bare target.
- Damage is rounded **half-up to an integer** after mitigation (`TgEffectDamage.uc:167`), which is
  why log figures are exact.
- The reported "mitigated" figure is `pre − post` computed **after** the health cap, so it is not a
  pure mitigation number whenever the cap fires.

## 9. Instrumentation

Every log site for this is on one channel, `"ki"`, enabled via `control-server.json`. It prints
prop 155's `m_fRaw` at capture, at lend-back and at restore, per shot. Silent when the channel is
off.

---

## 10. Downstream — the theorycrafting console

No console change was needed. `GA.mitigate` already implements the health cap correctly
(`healthCapArmed`, players only, floor `ceil(maxHP × 10%)` — backlog G1.3) and reports mitigation
as `raw − dealt`, matching the game. The leak flag that backlog C3 anticipated was never actually
written, so there is nothing to remove — the console models post-fix behaviour as-is.

`damage-pipeline.md` and `backlog.md` C3 have been corrected to match this file.
