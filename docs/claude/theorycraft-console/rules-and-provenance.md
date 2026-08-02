# Rules, and where they live

This answers one question: **if the game changes tomorrow, does the console follow?**

The short version is that there are three layers, and only the first follows on its own.

| Layer | Changing it means | Who has to act |
|---|---|---|
| **1. Data** — every number | re-run the generators | you |
| **2. Rule layer** — how numbers combine | edit `bench.js` / `builder.js` | a code change |
| **3. Provenance** — why we believe the rule | nothing changes automatically, ever | someone has to notice |

Layer 3 is the one with no safety net, and the reason this file exists.

---

## 1. Data — regenerates, no code change

The console is a single ~4 MB static HTML file with the game data **baked in**. It is not
live. A data change is picked up by re-running the generators and redeploying:

```bash
python gen2.py && python gen_ix.py && python gen_char.py && python gen3.py
```

`gen2` builds the device model, `gen_ix` the skill interactions, `gen_char` the accounts, and
`gen3` assembles the page. All four read `E:\GA_LOCAL\gaa.db` — the path is hard-coded, so
today only that machine can rebuild (backlog `T1`).

**Worked example — buffing Killer Instinct.** Every number it uses maps 1:1 to a column:

| Column | Value | Becomes |
|---|---|---|
| `asm_data_set_effects.base_value` | 10.0 | `fx: [[155,10,70,0]]` |
| `effect_groups.lifetime_sec` | 3.0 | `life: 3` |
| `situational_type_value_id` / `situational_value` | 1271 / 75.0 | `sit: 1271, sv: 75` |
| `category_value_id` | 302 | `cat: 302` |
| `application_value_id` / `application_value` | 836 / 10 | `app: 836, appv: 10` |

Change the debuff to −15, or move the gate from 75% to 60%, or shorten it to 2s, and the
console follows on the next regenerate. **No patch needed.**

The same is true of every effect magnitude, lifetime, tick interval, category and stacking
rule in the game, plus device power costs, refire, cooldowns, deploy times, skill trees and
their prerequisites, item mod rolls, inventories and characters.

---

## 2. Rule layer — needs a code change

These are hard-coded because they are *classifications and arithmetic*, not values. If the
game introduces a new one, or changes what an existing one means, the console will keep
applying the old rule and say nothing.

| Rule | Where | Constant |
|---|---|---|
| Which effect types land on someone else | `bench.js` | `LANDS_ON_OTHER {264, 272, 505, 398}` |
| Which are always-on once allocated | `bench.js` | `ALWAYS {261, 283}` |
| Situational condition **kinds** | `bench.js` | `SIT_BELOW 1270`, `SIT_ABOVE 1271` |
| Categories players are immune to | `bench.js` | `PLAYER_IMMUNE {653, 921}` |
| Category that never contends | `bench.js` | `NOT_A_BUCKET {302}` |
| Shield pool category | `bench.js` | `SHIELD_CAT 770` |
| Penalty that stays on the carrier | `bench.js` | `SELF_PENALTY {1452}` |
| How each stacking rule resolves | `bench.js` | `155 / 156 / 157 / 836 / 874` |
| Damage-type → protection prop | `bench.js` | `DMGTYPE_PROT` |
| Attack-type → protection prop | `bench.js` | `ATKTYPE_PROT` |
| Category → protection prop | `bench.js` | `CAT_PROT` |
| Which categories get the attack-type axis | `bench.js` | `NORMAL_CATS {302, 963}` |
| Protection rating denominator | `builder.js` | `REF_RATING 100` |
| Anti-one-shot floor | `bench.js` | `0.10` of max HP |
| Global off-hand cooldown | `builder.js` | `OFFHAND_GCD 1.0` |
| Simulation step | `builder.js` | `STEP 0.1` |
| Standard boost cost | `builder.js` | `MORALE_BASE 15840` |

A **new situational type** is the most likely thing to catch us out. `1270`/`1271` are health
gates; a condition keyed on anything else would be read as "no gate" and the effect would
apply unconditionally.

---

## 3. Provenance — what each rule is actually based on

This is the part nothing else records. A rule can be right, wrong, or *silently outdated*,
and the console cannot tell the difference. Confidence is graded:

- **UC** — read out of the decompiled UnrealScript
- **Server** — implemented in this repo, so the console and the server can be diffed
- **Measured** — established by an in-game test
- **Inferred** — reasoned from the data, never confirmed against code or a test

### Well grounded

| Rule | Basis | Citation |
|---|---|---|
| Three protection axes, multiplied not summed | UC | `TgEffectGroup.CalcProtection` |
| Protection integer-floored; ≥ rating is immunity | UC | `TgEffectGroup.uc:745` |
| Exactly one attack-type axis (AOE replaces Ranged) | UC | `CalcAttackTypeProtection` |
| Damage rounded half-up after mitigation | UC | `TgEffectDamage.uc:167` |
| DoT: attacker-side scaling cached at application, reused every tick | UC | `TgEffectDamage.uc:103/136/137/142` (`m_fBuffedDamageInitial`), §23 |
| DoT: target-side modifiers re-evaluated on every tick | UC | `TgEffectDamage.uc:144` `CheckDamageTakenModifier`, §23 |
| **Strongest Wins re-application RESTARTS a burn's tick clock** | Server | `RemoveAllEffectGroups` step 1 cancels every timer armed on the group, then the caller clones a fresh one. A weapon swinging faster than the tick interval therefore never lands a tick while it keeps swinging. |
| A burn's application rule decides what re-applying does | Server + UC | 157 displace+restart · 156 displace · 836 refresh in place · 155 separate instance · 874 incoming dropped |
| Additional-damage-taken applied before mitigation | UC | `TgEffectDamage.CheckDamageTakenModifier` |
| Anti-one-shot cap: floor `ceil(maxHP × 10%)`, arms only at full HP | UC | `TgDeviceFire.ApplyHit:1287` |
| Which effect-group type lands on whom | UC | `TgDevice.uc` constants + `SubmitHitEffects` call sites |
| Backstab is situational type 509 on a melee 505 group | UC | `GetSituationalMeleeEffectToApply` (509 backstab / 718 block-breaker) |
| Global off-hand cooldown 1.0s server-side | UC | `TgDevice.uc:1390/1400`, gate at `:2587` |
| Shields drain by the protection they absorb | Server | `TgEffectManager::SubmitMitigationDamage` |
| **Strongest Wins ranks on `application_value`, ties broken by lifetime** | Server | `TgEffectManager__IsStrongest.cpp` — caller is `GetNewEffectGroupByApp` case 157 |
| Category 302 is scoped by effect-group id, not category | Server | same file; also `GetStackingEffectGroup` uc:464, `GetRefreshedEffectGroup` uc:504 |
| Output mod is its own multiplicative layer | Measured | backlog `C1`, matched to the unit on both shots |
| Power regen does not run while power is being spent | Measured | backlog `C4` |
| Deploy time is `prop 279 ÷ (1 + DeployRate)` | Measured | backlog `C2` |

### Inferred — the risk list

These are held up by reasoning about the data, not by code or measurement. They are the ones
most likely to be quietly wrong, and the first place to look when the console disagrees with
the game.

| Rule | What it rests on | How it could be wrong |
|---|---|---|
| **Players are immune to categories 653 / 921** | every effect in both is a penalty, and `gen2` names them EMP Critical Failure / EMP Burn | if either category is ever used for something that touches players, they would be wrongly immune |
| **A damage chip with an apply interval ticks once per interval for its lifetime** | every such group in the data is `interval 1.0` with the tick flag set | the number of ticks, and whether the first lands immediately or after one interval, is assumed |
| **Refresh (836) preserves a burn's tick schedule** | `GetRefreshedEffectGroup` (uc:504) extends the group in place rather than replacing it, so nothing cancels its timer | if it re-arms the timer, Refresh burns would behave like Strongest Wins ones and tick far less |
| **Newest Wins (156) restarts the tick clock** | `GetStackingEffectGroup` (uc:464) is a displacement path, so assumed to behave like 157 | not read directly; no server reimplementation to compare against |
| **A splash radius on a ranged attack selects the AOE axis** | grenades declare "Projectile Ranged Attack" like the Ballista, so radius is the only discriminator | a different flag may drive it |
| **A starved device waits for a usable pool before firing again** | pure modelling choice; no data behind the threshold | affects sustained DPS directly; flagged for an on-screen control |
| **Regen suppression covers the firing interval, capped at 1s** | the cap is invented; without it a 60s cooldown would suppress regen for 60s | changes time-to-empty on every power weapon |
| **Support devices re-fire when their effects expire rather than at refire rate** | reasoning about how a player actually uses them | changes rotations |
| Morale accrual rate | not derivable — props 326/398 unused, `AddMoralePoints` is a native with no symbol | the whole boost timing model rests on a calibration, not a formula |

### Deliberate divergences

Known, chosen, not bugs:

- Backstab power drains are gated behind the tick box, and the timeline has no positional
  model — nothing decides whether you are actually behind the target.
- Grenades do not damage the thrower.
- Everyone commits at `t=0` unless a device is dragged.
- Shield firing order is slot order.
- Live buffs apply as a fire-time multiplier, not a full re-resolve (`G1.4` limitation).
- TheoryCrafter armour is preset configs, not per-piece rolls — so hand-built armour has no
  inventory row and never exports.
- Effects lingering across a weapon swap are not modelled (backlog `D6`).

---

## Drift hazard

The server is the other implementation of most of section 3. When someone changes
`TgEffectManager__IsStrongest.cpp`, or the mitigation order in `TgDeviceFire`, **nothing tells
the console**. There is no shared source and no test that compares them.

Until there is, the practical protection is this file: when a combat rule changes server-side,
check whether it appears in the tables above, and if it does, change both.
