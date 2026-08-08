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
now scales `fThreat` by the attacker's buffed Threat Modifier via the intact
`GetBuffedProperty` (`0x109d7ff0`), with origin-resolved device instance +
class skill so the gated entries (Assault Melee III, Recon Rifle Damage) only
scale their own weapon family — the exact query shape of the proven
`CheckEffectBuffModifier` reimplementation. `bUsePotencyModifier=0` (the
damage feeding fThreat was already potency-scaled upstream).

The query must ask for **prop 420 THREAT, not 421 directly**:
`ConvertPropToPropList` (`0x109e5220`, vtable +0x570) has no srcType-1 row
for 421 — a direct 421 request expands to an EMPTY prop list and the buff
walk no-ops — while `case 420: emit(421)` is the canonical expansion that
reaches the pawn's 421 buff entries. Found live 2026-08-06: the pawn's
`m_EffectBuffInfo` held both carriers ({421, skill=0} +15 Super Tank,
{421, skill=366} +50 Assault Melee III) yet 421-direct queries, gated and
ungated alike, returned the value unchanged.

**Verified live 2026-08-06**, all four carriers:

- Assault with Super Tank (+15% ungated) and Assault Melee III (+50% gated
  skill 366): guns/Overcharge ×1.15, melee ×1.65 — additive within the
  skill layer, and the {skill=366} entry only joined when the query's
  device skill was 366.
- Recon with Recon Rifle Damage (−10% gated skill 327) and a Decoy out:
  rifle hits ×0.90, rising to ×0.72 while the Decoy's 10s firing buff was
  up — 0.90 × 0.80, NOT −30% additive: the Decoy buff is effect-applied,
  lands in the SELF layer, and multiplies against the skill-layer result
  (`CheckBuffInfoList` @0x109cd4a0 three-layer formula).

To re-verify, enable the `threat` log channel and damage any AI bot with a
421 carrier active — expect `[THREAT-MOD] threat X -> Y` lines with Y/X
matching the modifier.

## Where threat is NOT the mechanism (2026-08-06 mapping)

The aggro pipeline: engine perception events → controller memory flags +
`m_bInterrupt` → data-driven behavior tree (`asm_data_set_bot_behaviors` /
`_bot_actions` / `_bot_tests` in gaa.db, evaluated by the native
`ChooseNextAction`) → the installed action row names its target source
(`target_type_value_id`) → the `SetTarget` switch (`TgAIController.uc:2710`).
**Threat is consulted only when the installed row explicitly asks for it** —
target code 818 "Highest Recent Threat" or tests 979 / 1264 / 1265 / 826 / 827.

Numbers (gaa.db): **346 bot definitions, only 100 ever reference threat**
anywhere in their tree (all seven `Boss *` defs, colony/legion elites,
champions). Of ~2536 action rows, 80 target 818 — versus 243 "Nearest Enemy"
(3), 96 "Last Attacker" (228), 56 "Friend's Target" (689), 31 "Nearest
Taunter" (835). Turrets (beh 103), Decoys (352), Eyes (369), Alarm Responders
(632), pets (600), and most mission trash never touch threat at all.

Systems that supersede or precede threat:

- **Activation / proximity aggro**: threat cannot activate anyone (list is
  empty out of combat; confirmed by the 2026-07-20 SuperAgent seed playtest).
  First contact is engine sight/hearing → `SeePlayer` stamps
  `m_fLastSawEnemy` + interrupt (`TgAIController.uc:1079`), `HearNoise`
  stamps `m_pLastSoundActor`. Trees gate on "Enemy Recently Sighted" /
  "Distance From Nearest Enemy" / "Number Of Enemies Sighted" and typically
  pick **Nearest Enemy** — distance, not threat. Per-bot perception config in
  `asm_data_set_bots`: `default_sensor_range`, `default_aggro_range`,
  `hearing_range`, `stealth_sensor_range`/`stealth_aggro_range`,
  `fixed_fov_degrees`, `chase_range`, `chase_time_sec`.
- **Damage response**: `NotifyTakeHit` (`TgAIController.uc:1151`) stamps
  `m_pLastAttacker` + interrupt; trees keyed on "Recent Damage" / "Has Last
  Attacker" pick code 228 **Last Attacker** directly. `AddThreat` accrues in
  parallel but only matters when a later row asks for 818.
- **Pack aggro (help calls)**: behavior action 249 → `CallForHelp` →
  `TgPawn.CallBotsForHelp` (`TgPawn.uc:4149`) sets `m_pFriendToHelp` on
  every friendly AI within its `m_fHelpRange` (|ΔZ| < 200, same
  sound-insulation volume) + interrupt. Helpers then target 254 Friend To
  Help / 255 Friend's Attacker / 689 Friend's Target. No threat transfer.
- **Squads**: roles 1404 leader / 1405 follower wired by `LookForSquad`
  (spawn-location proximity within the leader's help range, same task
  force); followers target via 313/314/344/1408 (owner's attacker/target).
- **Taunt — a separate channel from threat**: pawn-side `s_fTauntAmount > 0`
  makes `SeePlayer` stamp `m_fLastSawTaunter`; trees use code 835 Nearest
  Taunter (**distance-ranked**, `FindClosestTaunter` 0x10a81fc0) and tests
  834/1257/1600. This is how Decoys and taunt pets pull bots that never read
  a threat list. (Threat prop 420 injection is a different thing — it feeds
  the ledger.)
- **Alarms**: behavior action 620 → `RadioAlarm` — a stripped stub at
  0x10a7e890 (same six-stub block as SpawnPets/SetBotTeam/SetTaskForceNumber),
  reimplemented server-side (`TgAIController__RadioAlarm.cpp`) delegating to
  the intact `TgGame::ActivateAlarm` (0x10a75740): fires the AlarmBots kismet
  events and drives the closest `bSpawnOnAlarm` factory. Responders spawn
  with `m_bAlarmBot=1` and originator/trigger targeting; test 1159 "Is Alarm
  Bot" (85 rows) drives their stand-down/despawn actions. The Alarm
  Responder def (beh 632) never uses threat.
- **Kismet / mission**: `TgSeqAct_TriggerBots` → `m_pTriggerTarget` (code
  288); objective codes 1402/1491/1492; command target 710 (crew control).
  Factory-spawned attackers/defenders get `m_pTriggerTarget` pre-seeded to
  the objective bot (`TgBotFactory__SpawnNextBot.cpp`).
- **Encounter volumes / boss rooms**: `TgBotEncounterVolume.CheckTouching`
  starts/ends factory encounters purely on human-player presence in the
  volume — population control, orthogonal to targeting. Hibernation
  (`hibernate_on_idle_sec`) is an idle gate on top.

Lifecycle boundaries — even for the 100 threat users, the ledger never
outlives an engagement: `OnExitCombat` clears `m_ThreatList` outright
(`TgAIController.uc:987`), the list is cleared when the bot's pawn dies,
entries are pruned on victim death, and 10%/s decay erodes the rest. Every
fresh engagement therefore begins on perception/help/alarm mechanics; threat
only reorders targets *within* an ongoing fight, on bots whose tree rows ask
for it, in the slots where they ask for it.

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
