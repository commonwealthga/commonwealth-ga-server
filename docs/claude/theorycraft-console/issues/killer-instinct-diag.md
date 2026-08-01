# Killer Instinct self-shot leak — fix handoff

**Purpose:** this file is self-contained. Carry it into a fresh chat on the server repo to work
the fix. You should not need the theorycrafting console or the conversation that produced this.

**Status:** confirmed a bug by the project owner (2026-08-01). A fix is wanted. This is *not* a
"decide whether it's intended" investigation — that question is closed.

**Companion:** [killer-instinct-self-shot-leak.md](killer-instinct-self-shot-leak.md) is the
player-facing bug report (test conditions and raw numbers). This file is the engineering side.

---

## 1. The bug

**Killer Instinct** is a Recon skill: hit a target above 75% HP with a rifle and their Physical
protection drops 10 for 3 seconds. It is an *on-hit* debuff, so it should only affect **subsequent**
shots — the shot that triggers it must be mitigated against the target's full protection.

It doesn't. The triggering shot lands **50 damage harder than it should** — and §6 shows that this
is the *full* −10 reaching about 31% of the shot, not a fraction of the debuff reaching all of it.

The Ballista's own on-hit debuff does **not** do this; its triggering shot stays clean. The two
differ only in dispatch type — Killer Instinct is **type 505 (Hit-Situational)**, the Ballista
debuff is **type 264 (Hit)**.

---

## 2. The evidence

Controlled A/B, Ballista OC at point-blank against a live player with **no armour and no skills**
(base Physical protection = 30), at full HP before shot 1 of every run. When a skill was removed
for a run the freed point went into a random Balanced-tree node, never into Ranged Damage, so
offence stayed constant and only the defensive side moved.

Mitigation is 1:1 with Physical protection on this build, and raw shot-1 damage is 1600.

| Build | Shot 1 dealt | Shot 1 mitigated | ⇒ apparent protection |
|---|---|---|---|
| Killer Instinct slotted | **1170** | 430 | 26.875 — **not achievable, see §6** |
| Killer Instinct removed | **1120** | 480 | **30** (clean) |

**The leak is exactly 50 damage on shot 1.** The "26.875" is what you get by treating mitigation as a
single continuous protection value; §6 shows the engine cannot produce it, which is what identifies
the real mechanism.

Shot 1 is the control here: every on-hit debuff in the build is supposed to apply *after* it, so
both rows should read 30. Only the row with Killer Instinct doesn't.

Note it is **partial**. A whole-debuff ordering error would put shot 1 at protection 20 (1280
dealt). It doesn't — so "the effect simply applies too early" is not a sufficient explanation on
its own.

---

## 3. Settled — do not re-derive, and do not "fix"

Two findings came out of the same test and are **correct behaviour**. It would be easy to
"tidy" them while in this code and cause a regression:

- **Eagle Eye's potency amplifies the Ballista's type-264 device debuff (−10 → −13) but does
  NOT amplify Killer Instinct.** Confirmed intended by the project owner. It is emergent, not
  special-cased: Eagle Eye's prop-376 potency registers in `m_EffectBuffInfo` scoped to
  `nReqSkillId = 327` (Recon Rifles); the device debuff carries skill 327 as its origin and
  matches, while Killer Instinct is skill-sourced with `m_nSourceDeviceInstId = 0` so the potency
  query runs with `skillId=0` and finds nothing (`TgPawn__ApplyBuff.cpp`: a stored `skillId > 0`
  only matches the same skillId). **Leave this alone.**
- **The HP gate re-evaluating per shot is intended.** Shot 1 applies Killer Instinct (target at
  100%); on shot 2 the target is below 75% so it does not *re-*apply, but shot 1's instance is
  still live for its 3s. That is the model working.
- **Mitigation % = Physical protection value, 1:1** on this configuration. An earlier
  "mit% ≈ protection − 3" reading was this very leak contaminating the baseline.

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
| Ballista OC (the clean comparison) | device **2110**, mode 2618, debuff eg **18975** — type **264**, prop 155 −10, 5s, class 80, category 986 |
| Eagle Eye (do not disturb) | skill 834, eg 17244, type 261 Equip, prop 376 potency |

Enum decode: effect_group_type **264** = Hit · **505** = Hit-Situational · **759** = skill
self-buff · **261** = Equip. situational_type **509** = generic on ranged hit · **1271** = target
HP above `situational_value`% · **1270** = below.

---

## 5. Where the code is

The dispatch that matters is **UnrealScript bytecode, not in the exe** — you will not find it by
reading Ghidra output. Read it from the decompiled source repo:
**github.com/commonwealthga/ga-source** (full UnrealScript; this is the reference for any
`.uc`-cited function on this project).

```
TgDeviceFire.SubmitHitEffects(instigator, impact, eSource, nType, nSituationalType)   [UC]
  ├─ nType 264  Hit             → target: base damage (TgEffectDamage, class 181)
  │                                        + plain on-hit debuffs   ← Ballista debuff, stays clean
  ├─ nType 505  Hit-Situational → ShouldSituationalApplyEffect(target, eg)   [UC, evaluates HP>75]
  │                                 └─ pass → SubmitEffect → target.ProcessEffect
  │                                            → Killer Instinct −10 protection   ← THE LEAK
  └─ nType 759  skill self-buff → GetSkillBasedEffectGroup [our native] → attacker.ProcessEffect
```

**The question to answer first:** where in `SubmitHitEffects` does the type-505 branch run relative
to the damage calculation that consumes the target's protection? If 505 is submitted before
`CalcProtection` reads prop 155 for this impact, the triggering shot sees the debuffed value.

That explains a leak. It does not by itself explain a **partial** one — for that, see §6: the shot's
damage appears to be submitted in more than one piece, and the 505 debuff lands between them.

---

## 6. Why 3.125 and not 10 — SETTLED

**The leak is the full −10 applied to part of the shot, not a fraction of the debuff applied to all
of it.** Established 2026-08-01 from the decompiled `CalcProtection`, which the earlier analysis
predates:

```
nProtection = int( FClamp(prop.m_fRaw, m_fMinimum, m_fMaximum) )   // INTEGER-FLOORED
```

Protection is floored to an integer before the divide, so at rating 100 the reachable shot-1 values
either side of the measurement are:

| protection | dealt |
|---|---|
| 26 | **1184** |
| 27 | **1168** |
| *measured* | **1170** |

There is no protection value that produces 1170, and the figure is exact — the combat log only ever
emits whole numbers, because damage is rounded half-up right after mitigation
(`TgEffectDamage.uc:167`). So **"Killer Instinct shaves ~3 points off protection" is impossible.**
Both the flat and the proportional readings are dead.

What fits exactly is staged damage. With a fraction *f* of the shot mitigated at protection 20 (the
full −10 applied) and the rest at 30:

```
dealt = 1600 × [0.70 + 0.10f] = 1120 + 160f     →  1170 gives f = 0.3125
```

Every quantity there is an integer protection, so this reproduces the measurement with no rounding
slack: 500 damage mitigated at 20, 1100 at 30, total 1170.

**What this means for the fix.** Do not go looking for a partial protection value or an off-by-one in
the debuff magnitude — look for **the shot's damage being submitted in more than one piece**, with the
type-505 debuff landing between the pieces. The 0.3125 split is the thing to explain.

## 7. Confirmed from source, so do not re-derive

Read from `github.com/commonwealthga/ga-source` (private; UC under `unrealscript/`, Ghidra C++ under
`decompiled/`):

- `TgEffectGroup.CalcAttackTypeProtection` is a **switch** — exactly one attack-type axis ever applies
  (3 → 219 AOE, 1 → 217 Melee, 2 → 218 Range). They never stack.
- `TgEffectDamage.ProtectionModifier` order: Hunter → Category → DamageType → AttackType, the last only
  for `m_nCategoryCode` 302 or 963.
- `m_eAttackType` is **never assigned in UnrealScript** — populated natively, so the UC will not tell
  you what sets it. Empirically a ranged attack with a splash radius (prop 6) resolves as AOE.
- Damage is rounded **half-up to an integer** after mitigation, which is why log figures are exact.

## 8. Instrumentation

Per the repo's logging rules: put **every** log site for this on **one** channel — use `"ki"` —
and enable it via `control-server.json`, not `dllmain`. Do not ask for a multi-channel capture.

Worth logging, in impact order, for a single shot:

- entry to each `SubmitHitEffects` branch, with `nType` and the effect group id
- the `ShouldSituationalApplyEffect` verdict for eg 16596 and the HP it read
- the target's prop 155 value **immediately before** the damage calculation consumes it, and
  again immediately after the type-505 submit
- the final mitigation applied to that impact

The decisive line is prop 155 at damage time versus after the 505 submit. If they differ on the
triggering shot, the ordering is confirmed and the log will also show *how* the partial value
arises.

---

## 9. What "fixed" looks like

Shot 1 mitigated against protection **30**, dealing **1120** in the rig above — matching the
Killer-Instinct-removed row exactly. Shot 2 is unchanged (the debuff is still live for it, so the
full build should still read 1548). Re-run all three A/B rows; the "no Killer Instinct" and
"no Eagle Eye" rows must not move.

**Regression risk to watch:** the fix must not disturb Eagle Eye's asymmetry from §3. After the
fix, Eagle Eye must still amplify the Ballista's type-264 debuff to −13 and must still leave
Killer Instinct at −10.

---

## 10. Downstream — the theorycrafting console

Until this is fixed the console models the leak **as it currently behaves**, so its numbers match
what players actually experience. It is scheduled to be deleted, so it belongs behind one named
flag in the mitigation stage (backlog **G1.1**), not scattered through the arithmetic.

**When the fix ships, tell whoever maintains the console** — the flag comes out, and
`docs/claude/theorycraft-console/damage-pipeline.md` ("SECONDARY QUIRK") plus `backlog.md` C3
need updating.

---

## Repo rules that apply to this work

From `CLAUDE.md`, easy to trip over on a first session:

- **Never build the project.** Static re-reading is the verification.
- **Never commit.** The human controls all commits.
- Debug logs never go to `GetLogChannel()` — that drives the call-tree visualiser.
- The UnrealScript client is unmodified retail; the fix is server-side.
