# Threat — how AI aggro works, and what was dead in the shipped binary

Findings from the 2026-08-06 investigation (UC source + Ghidra on the live
`GlobalAgenda.exe` + gaa.db). Every address below was read or decompiled, not
inferred.

## The mechanic

Threat exists **only on `TgAIController`** — a per-AI ledger `m_ThreatList` of
`{pawn, fThreat}` pairs (`this+0x56C` data / `+0x570` count). Player
controllers have none, so all of this is PvE/bot-only.

| stage | where | behaviour |
|---|---|---|
| Generation (damage) | `TgEffectDamage.uc:241` | threat = health the hit actually removed (overkill excluded; mitigation reduces threat along with damage), routed through `CheckEffectThreatModifier`, then `AddThreat(attacker, fThreat)` |
| Generation (direct) | `TgEffect.uc:388` | an effect carrying **prop 420 THREAT** injects its raw amount — a pure taunt. One user in all of data: Overcharge's Nano Attack, +4000 per hit (egt 264) |
| Generation (healing) | `TgEffectHeal.uc:185` | every heal calls `AddHealingThreat(health restored)` — **stubbed, see below** |
| Accumulation | `0x10a84fa0` (intact) | find-or-append the attacker, `entry.fThreat += amount`. No cap, no clamp; negative amounts work (SuperAgent uses that to retract seeded threat) |
| Decay | `0x10a87b30`, per tick | every entry ×= `(1 − rate·dt)`; rate = config **`ThreatDecayPercentPerSecond`, default 10** (not in gaa.db — an engine/INI read). Entries pruned below 0.001 or on pawn death; whole list cleared when the AI's own pawn dies |
| Target pick | `GetHighestThreat`, `0x10a80d20` | **bare max-scan — highest number wins.** No distance, no LOS, no recency in the pick. Empty list falls through to a default-target virtual (`vtable+0x440`) |

Distance/LOS enter only as **boolean gates on the already-chosen #1**, run by
the behavior tree, never as re-ranking:

- `HighestThreatInLOS` (`0x10a860c0`): `GetHighestThreat()` then one LOS trace
  to that pawn.
- `HighestThreatInDeviceRange` (`0x10a7eba0`): `GetHighestThreat()` then a
  range test of that pawn vs an equip slot's device.

A top-threat player behind cover keeps the #1 slot the whole time (only decay
erodes it); the bot's tree merely routes around the failed gate meanwhile.
Threat also only *steers* a bot already in a combat behavior — seeding threat
did not activate the SuperAgent elites (playtest 2026-07-20, see
`SuperAgent.cpp` kSeedQuarryAggro).

Steady-state intuition: at the default 10%/s decay, sustained fire plateaus at
about **10× your threat-per-second**. Aggro is therefore "most damage in the
last several seconds", not total damage this fight.

Neighbouring decompiles that are NOT threat: `FUN_10a87710` / `FUN_10a87870`
contain distance math but are AI perception natives (enemies-within-radius
style); don't let the adjacency mislead.

## What shipped broken: the stripped-stub block at 0x10a6f270

- **`CheckEffectThreatModifier` = `RET 4` stub at `0x10a6f280`** → damage
  threat was never scaled, making **prop 421 Threat Modifier a dead stat**.
  Its in-data carriers (all percentages): Decoy −20% ×10s while firing
  (category 1601, the category's only occupant), Assault Melee III +50%
  (gated Assault Melee), Super Tank +15%, Recon Rifle Damage −10% (gated
  Recon Rifles) — plus a skill extending category 1601's duration.
- **`AddHealingThreat` = `RET 4` stub at `0x10a6f2e0`** (traced via the
  `UTgEffectHeal` vtable) → **healing generates zero threat**, despite the UC
  charging it on every heal. Healer aggro was designed and never shipped.

Intended semantics of 421 (confirmed multiplier, not a floor): threat per hit
= damage × (1 + Σ modifiers). Zero damage stays zero threat — no skill grants
a standing threat level. Decay is untouched by any modifier.

## The fix on this branch

`TgEffect__CheckEffectThreatModifier` (previously a deliberate pass-through)
now scales `fThreat` by the attacker's buffed prop 421 via the intact
`GetBuffedProperty` (`0x109d7ff0`), with origin-resolved device instance +
class skill so the gated entries (Assault Melee III, Recon Rifle Damage) only
scale their own weapon family — the exact query shape of the proven
`CheckEffectBuffModifier` reimplementation. `bUsePotencyModifier=0` (the
damage feeding fThreat was already potency-scaled upstream).

Verification: enable the `threat` log channel and damage a bot with a 421
carrier active — expect `[THREAT-MOD] threat X -> Y` lines with Y/X matching
the modifier (e.g. Super Tank melee hit → ×1.65 from +15% ungated +50% melee-
gated; Decoy firing → ×0.80).

## Open items

- **`AddHealingThreat` is still a stub.** Reimplementing it is design, not
  archaeology — the stub means there is no original behaviour to recover.
  Plausible shape: credit the healer on every AI whose threat list contains
  the healed pawn, amount = health restored (possibly scaled); recipients and
  scale need the owner's decision before any code.
- `ThreatDecayPercentPerSecond` override location (which INI section) not yet
  pinned down; default 10 applies unless one is found.
- The console/reference `polarity?` flags on Threat lines: as-designed the
  stat is intent-dependent (tank wants more, recon wants less), so the amber
  flag stays even now the modifier works.
