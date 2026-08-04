# Device stats → perf engine: integration plan

Response to `device-stats-mmr-handoff.md` (on `device-usage-metrics`). What the
new signals change for the rating engine in `src/ControlServer/MmrService/`,
what has to be built before they can be consumed, and what cannot be decided
until data exists.

**Status: plan only.** Recording is pre-golive, so there are no rows to fit
against. Nothing here is tuned; the weights are placeholders with a stated
procedure for setting them.

The archetype definitions in §4 are now specified rather than open — they came
from the game side, not from the data, and the device ids were resolved against
the canonical inventory. §7 records what that settled and what it did not.

---

## 1. Why this matters more than it looks

The engine's standing limitation, stated on the player-facing page and in the
review doc, is that it measures what is in `ga_match_player_stats` and nothing
else. Two experienced medics — amna (81 games) and Callizle (78) — sit mid- and
low-table with the same profile: **good at surviving, average at every measured
output**. Players who know them say they are better than that. The honest
conclusion was that whatever they do well is not healing volume, assists, buffs
or objective score, because all four are measured and they are average on them.

The device signals are the first data that could settle it. `rescues`,
effective-heal ratio and `debuffs_removed` are precisely the "clutch and
discipline" axes that raw healing cannot see.

There is a second reason, from the closed-pool analysis (`tools/mmr/closed_pool.py`):
thirty regulars account for 92% of appearances and any two are teammates 47% of
the time, so **only ~45% of the variance in win rates is skill**. Win/loss
cannot carry a rating here. Per-player device measurements do not dilute across
teammates the way a match result does, which makes them structurally more
valuable in this population than they would be on a larger server.

---

## 2. What has to change in the engine first

The current pipeline makes four assumptions the device data breaks, and needs a
fifth thing it does not have at all (2e). These are prerequisites, not
refinements.

### 2a. Stats come from more than one table

`ClassProfiles::StatColumn` returns a `ga_match_player_stats` column or `nullptr`
for derived, and `BuildParticipants` builds one `SELECT` from it. Device signals
need a second source with its own aggregation and a `LEFT JOIN` on
`(instance_id, character_id)`.

```
enum class Source { PlayerStats, DeviceStats, Derived };
```

### 2b. Not everything is a per-minute rate

Every stat is currently divided by minutes played. That is right for volumes and
wrong for ratios — effective-heal ratio, boost reach (`boost_targets / uses`)
and boost freshness are already normalized, and dividing them by time makes them
meaningless.

```
enum class Scaling { PerMinute, Ratio };
```

### 2c. Missing is not zero — and this one will bite silently

Matches before recording started have no device rows. If those are folded in as
zeros, the class baselines are dragged down and **every player who was active
before golive looks catastrophically bad at rescues and cleanses**, while
post-golive players look inflated against a corrupted mean. The engine would
produce a confident, wrong answer.

`Participant` needs per-stat availability, and both scoring *and*
`AccumulateBaseline` must skip unavailable stats. The structures already support
this — `ZScore` returns false on insufficient baseline, and `ArchSignal` tracks
per-stat counts — so it is an extension rather than a rewrite.

The handoff says this explicitly ("absent rows = missing, not zero, when
normalizing"). It is the single easiest thing here to get wrong, because
nothing about the output will look broken.

### 2d. Devices need a family map — and it already exists, elsewhere

The effective-heal ratio must be restricted to heal-class devices, because
`overheal` includes on-hit rider heals — the handoff records a poison dagger
carrying ~14k overheal. Without the filter, a Death-Medic reads as the most
wasteful healer on the server.

Tiering is not a problem: **every device in play is max tier**, and the canonical
device list is one level-50 character's inventory (`ga_players_inventory`, user
2381 in the tooling). No tier collapsing is needed.

The taxonomy is not in the ASM tables in usable form — `item_subtype_value_id`
is 0 for every device checked, and `skill_id` does not partition by role (Frenzy
Wave and Protection Wave are both `skill = 1`). But it **does** already exist, on
`theorycraft-console`: `tools/device-console/gen_ix.py` emits `devmeta.json`,
carrying per device its `name`, `cls` (owning class), `heals`, `dmg`, `aoe`,
`pet` and `atk` flags — the console reports 115 usable devices, of which 19 are
healers and 15 are deployables/pets.

**Do not re-derive it.** I tried, filtering devices whose effect groups touch the
heal props `{330, 210, 51, 211}` — the same set the console uses for its `H`
chip letter — and got **73 "healers" including iMINIGUN, Tremor Launcher and
three grenades** against the true 19. Heal props appear in effect groups that
are not the device's own healing, and the console only reaches the right answer
after resolving fire modes, sign and context. A naive filter here would put the
rider-heal caveat back in through the front door, which is the exact failure it
was meant to prevent.

**Dependency, and it needs an owner.** `devmeta.json` is a generated artifact: it
is not committed, and `gen_ix.py` currently writes it to a hard-coded scratchpad
path from a previous session that no longer exists. Before the rating engine can
consume the taxonomy, one of:

- `gen_ix.py` emits a committed `device_taxonomy.json` at a stable repo path, or
- the classification lands in a small DB table the engine can join, or
- the flags move into `MatchStats.cpp` alongside the existing allowlist and
  exclusion list, so recording and rating share one definition.

The third is tidiest — `MatchStats.cpp` already carries a Group-Heals allowlist
and a device exclusion list, so a device taxonomy has a natural home there and
two copies cannot drift apart. **Decided: `MatchStats.cpp`.**

### 2e. Archetypes are decided by devices, not by outcomes

This is the largest change in the document and it was not in the earlier draft.

`ClassProfiles::Rule` compares *outcomes*: mean z of one stat group minus mean z
of another, above a threshold. Every archetype definition supplied for Recon,
Robotics and Medic (§4) is instead a statement about **which devices the player
used**. Those are different questions, and the device form is better for two
reasons:

- **It is an observation, not an inference.** A tank is currently identified
  partly by taking damage, which is also part of what a tank is *scored* on. The
  discriminant and the score share an input, so a player having a bad game can
  be moved onto a different archetype by the same numbers that judge them.
  Device usage breaks that loop: what you brought and fired is independent of
  how well it went.
- **It separates classes the current stats cannot.** A sword Recon and a sniper
  Recon produce `kills` and `damage_dealt` and nothing else the engine can see.
  Their device usage is not remotely similar.

So `Rule` needs a second form alongside the existing comparison:

```
struct DeviceRule {
    std::string      profile;
    std::vector<int> devices;    // item_ids
    Metric           metric;     // Uses | Damage | Kills
    double           min_share;  // of the player's total on this class
};
```

**Share of usage, never equipped loadout.** Stated twice and unprompted in the
spec: *"they would be using it in the match — that's important, because some
builds will just have it there and never use it."* There is no equipped-loadout
signal to be tempted by anyway; `ga_match_device_stats` records use, which is
the right thing.

Two consequences worth stating now:

- **Assignment stays per player, not per match**, for the reason already in the
  header comment — scoring someone on whichever profile flattered a given game
  inflates the whole population. Device shares accumulate across matches exactly
  as the z-means do today.
- **Devices are shared across archetypes and that is fine.** Stealth appears in
  three of the four Recon archetypes; it is a *corroborator*, not a
  discriminant. The discriminating device sets below are disjoint by
  construction — the offhands.

---

## 3. Signals, and how each is normalized

Volumes go per-minute like the existing stats. Ratios do not. Two are
deliberately excluded.

| Signal | Source | Scaling | Notes |
|---|---|---|---|
| `heal_eff` | `SUM(healing) / SUM(healing + overheal)`, heal-family devices only | Ratio | discipline vs wave-spam |
| `rescues` | `SUM(rescues)` | PerMinute | already self-excluded; already unifies Triage + Savior |
| `cleanses` | `SUM(debuffs_removed)` | PerMinute | self-cleanse counts by design |
| `dmg_enabled` | `SUM(buffed_damage_dealt)` | PerMinute | Frenzy — offensive buffing |
| `dmg_absorbed` | `SUM(protected_damage_taken)` | PerMinute | Protection/Savior — defensive buffing |
| `boost_reach` | `SUM(boost_targets) / SUM(uses)`, boost devices only | Ratio | placement |
| `boost_fresh` | `(BOOST_APPLY − BOOST_OVERWRITE) / BOOST_APPLY` from events | Ratio | was the cast justified |
| ~~`boost_overwrites`~~ | — | — | **excluded.** Being stomped is the other medic's timing. Not a covariate either — with 30 regulars it would proxy for "plays alongside the other main medic" |
| ~~SAVIOR events~~ | — | — | **excluded.** `rescues` already includes them; counting both double-counts |

### Normalization beyond per-minute

The handoff's caveat 2 — waves never affect their caster, so heal output depends
on having teammates nearby — is the same problem `rel_deaths` already solves for
deaths: measure against the team's situation, not in isolation.

Two candidates worth testing when data exists:

- **`rescue_share` = `rescues / (rescues + team_deaths)`** — the share of
  near-deaths converted into saves. This is the one I would expect to carry the
  most signal, because it is inherently normalized against how much trouble the
  team was in.
- **`heal_share` = `healing / team_damage_taken`** — already tested against the
  *old* data, where it was the single most reliable per-player measurement found
  (0.96) but nearly uncorrelated with winning (+0.049). Reliable-but-not-
  predictive is exactly the profile of a signal that measures effort rather than
  impact. Worth re-testing with the overheal split available, since the old
  version could not distinguish real healing from waste.

---

## 4. Archetype granularity, per class

Device stats are **not** here to invent archetypes. For Medic they identify the
existing three properly; for Recon and Robotics they make visible a split that
already exists in how the class is played and that the current stats cannot see
at all. An earlier draft proposed five or six medic archetypes; that was wrong
and is dropped.

```
class      today   proposed   why
Assault      2         2      roamer/tank already work; no change
Medic        3         3      same three, decided by devices not by z-scores
Recon        1         4      sniper / bomber / mines / melee
Robotics     1         2      drone / turret-station   ("two for now")
```

Device ids below are from the canonical level-50 inventory (`ga_players_inventory`,
user 2381) and are the discriminating sets — the ones a rule would key on.

### Recon — the class the current stats cannot see at all

The clearest win outside Medic. The engine currently sees `kills`,
`damage_dealt` and `bot_kills` for all four of these and cannot tell them apart,
so every Recon is scored against one profile that fits three of them badly.

```
sniper   ranged      2110 Ballista · 3249 Scorpia
         specialty   5807 Targeting System
         > discriminant: SHARE OF DAMAGE from Ballista/Scorpia.
         > the specialty corroborates; the ranged weapon decides.

bomber   offhand     2219 EMP Bomb · 4708 Venom Bomb · 3056 Fire Bomb
                     4716 Graviton Bomb  (rare)
         specialty   2209 Sprint Stealth · 3023 Spring Stealth
         > discriminant: SHARE OF USES from the bomb set. "throwing bombs
         > the whole match."

mines    offhand     2897 Sticky Poison Mine · 2225 Standard Mine
         specialty   stealth, as above
         > discriminant: SHARE OF USES from the mine set.
         > bomber and mines differ ONLY in the offhand — stealth is common
         > to both and carries no information for this split.

melee    melee       3970 Ghost Sword · 5799 Dual Daggers
         offhand     2953 Melee Stim          <-- the telltale
         specialty   stealth, as above
         > discriminant: SHARE OF DAMAGE from the melee slot, corroborated
         > by Melee Stim USES. Stim uses is the stated giveaway: "they would
         > have the melee stim equipped, and they would be USING it."
```

Two notes. *"Jewel daggers"* has no exact match in the item table; **Dual
Daggers (5799)** is the only dagger device and is assumed to be it — worth a
one-line confirmation, since Assassin Blade (6895) also exists and is not
mentioned. And Sprint Stealth (2209) and Spring Stealth (3023) are distinct
devices, each with I/II/III variants; both count as stealth.

Damage-share is the right metric for sniper and melee (a sniper's whole output
comes through the rifle), use-share for bomber and mines (mines can sit unused
and a bomb's damage varies with how many it catches).

### Robotics — a clean two-way split on the offhands

Currently one profile for the whole class. *"There is a very clear divide there
between the two in terms of the offhands they take."* Three-way (drone / turret
/ station) was considered and rejected: turrets and stations are carried by the
same players in the same builds.

```
drone         offhand     2107 Grizzly Drone · 2279 Hornet Drone
                          4782 Harrier Drone · 2675 Eye Drone ("iDrone")
                          4698 Lockdown Drone · 2051 Force Wall (sometimes)
              specialty   5811 Force Target
              > Force Target is a strong corroborator, not incidental: it
              > directs the drones onto a target, so it only makes sense in
              > this build.

turret/       offhand     2300 Personal Turret · 3755 Auto Cannon
station                   5792 Flame Turret · 2095 Rocket Turret
                          2066 Medical Station · 4076 Power Station
                          2326 Sensor
              specialty   2918 Focused Repair Arm
              > Focused Repair Arm is the corroborator — you bring it to keep
              > emplacements alive.
```

Discriminant: share of offhand uses. The two sets are disjoint, so this is close
to a straight majority vote. Force Wall (2051) is the one ambiguous device —
listed under drones but plausible in either; leave it out of the discriminating
set and let the drones decide.

### Medic — same three, decided by offhands rather than by `buff_value`

Full-heal, buff and poison stay. What changes is what decides them.

The specialty slot splits two ways but **does not separate the archetypes**:

```
nanite family   2061 Nanite Restoration System · 5064 Nanite Enhancement System
                6898 Adrenaline Gun            — heal-over-time, applied to others
beam family     2906 BioFeedback Beam · 3946 Boost Beam · 6004 Multi-Boost Beam
                                               — focused heal on a single target
```

That is a real distinction in *how* someone heals, and worth recording, but the
explicit instruction is that **the offhands tell the tale**:

```
clutch / lifesaver   5808 Triage Wave · 2376 Healing Wave · 2531 Healing Grenade
                     (+ perhaps one buff: Protection, Frenzy or Power Wave)

buff                 3639 Protection Wave · 4682 Power Wave · 3645 Frenzy Wave
                     nanite gun + nanite specialty, then buffs
                     "focused on giving buffs to the team"

poison               2379 Poison Aura · 2168 Poison Grenade · 4690 PowerVirus
                     Grenade · ranged 2991 Agonizer / 4676 Pain Gun
                     melee 3967 Poison Injector
```

The distinction is the *balance* of the wave offhands, not their presence — both
archetypes carry some of each. A clutch medic runs mostly heal waves with maybe
one buff; a buff medic runs mostly buffs with maybe a heal wave. So the rule is
a share comparison between the two sets, not a floor on either.

This replaces the current buff discriminant, which is a floor on `buff_value`
plus a healing floor — a blunt aggregate that says someone buffed a lot without
saying what they buffed or whether it landed. Alongside it,
`buffed_damage_dealt` and `protected_damage_taken` measure whether the buff did
any work.

**Full-heal / clutch medic** is where the amna / Callizle question lives. What
those players are believed to be good at is **wave discipline** — Triage on
low-health teammates, cleanses, Group Heal Savior procs, and *not* spamming
waves for volume. None of it is visible in raw `healing`, which is exactly why
they read as average.

| what it captures | signal |
|---|---|
| landing waves on people who needed them | `rescues` (self-excluded, already unifies Triage + Savior) |
| not spamming for volume | `heal_eff` = `healing / (healing + overheal)`, heal-family devices only |
| counterplay awareness | `debuffs_removed` |
| placement rather than luck | `boost_reach` = `boost_targets / uses` |

If this is right, amna and Callizle should score well on `rescues` and
`heal_eff` while remaining average on raw healing. That is a **falsifiable
prediction**, and the first thing to check when data exists — see §7.

**Poison medic — flagged for correction, by the person it favours.** The
archetype is unchanged, but its *valuation* is now a known open problem, raised
unprompted:

> "I am not better than a lot of the medics I was above, because my damage
> output as a medic is valued higher than their heals — which is arguable."

That is a self-report against interest and it matches a structural weakness
already visible in the data. Medic kills and damage are **zero-inflated** — most
medics record almost none — so their standard deviation is small and any medic
who deals damage lands several z above the mean, while a healer's `healing` z is
bounded by a population where everyone heals. The poison profile scores kills at
0.8 and damage at 0.8, directly on the two most inflated stats in the class.
Lowering the 0.45 premium treats the symptom; the cause is that a z-score
computed over a zero-inflated distribution is not comparable to one computed
over a well-spread distribution. Candidate fixes, in order of preference:

1. score poison medics' damage stats against the **poison sub-population**, once
   device usage identifies who they are — which is exactly what §2e provides;
2. use a rank or percentile transform instead of a z for zero-inflated stats;
3. failing both, reduce the premium — but that is a fudge and should be labelled
   as one.

The intent stated is to bake poison into the device tracker so their efficiency
is measured rather than assumed. Option 1 depends on that.

### Assault — unchanged

Roamer is a set piece and tank is already identified; the existing stats handle
both. Device usage could separate further styles, but nothing needs deciding
now.

### Deliberately deferred

Scoring a player against the *class average for the specific devices they use*
is a further step and may not prove useful. Not planned; revisit only if the
family-level split works first.

## 5. Weights — what I will not guess

Every weight in `ClassProfiles.cpp` today came from a test in `tools/mmr/`, and
several improvements that looked obvious in isolation turned out to be noise or
artefacts — learned per-stat weights performed *worse* out of sample, and a rule
built on a 66%-vs-49% win-rate split turned out to be two under-rated players
rather than a real pairing effect.

Setting device weights before there is data would repeat exactly that mistake.
The procedure instead:

1. **Wait for one full Sunday of recorded matches**, then check coverage — how
   many rated participants have device rows, and how many devices per player.
2. **Reliability first, not prediction.** Split-half each new signal per player
   the way `closed_pool.py` does. Anything below ~0.7 is not measuring a stable
   property of the player and should not be weighted regardless of how good the
   story is. Existing benchmarks: win/loss 0.64, performance index 0.90.
3. **Then out-of-sample prediction**, via the harness in `tools/mmr/reseed.py`,
   one signal at a time. Add a weight only if it improves prediction *and* is
   reliable.
4. **Re-check the archetype labels are stable.** With §2e this now means the
   device-share thresholds, not just the stat weights: does a player keep the
   same label between Sundays? Device shares should be far more stable than
   outcome-based rules, which is much of the argument for them — so if they are
   *not*, that is a finding and the threshold is wrong.
5. **Sanity-check the new splits against known players before trusting them.**
   Recon and Robotics go from one profile to four and two; the split is only
   worth having if the labels match what people actually play. This is cheap to
   check and catches a wrong device set immediately.
6. **Expect most candidates to fail.** Seven signals are listed; if two survive
   this procedure that is a good outcome.

Starting placeholders, to be overwritten by step 3 and not before: inside the
full-heal profile, `rescues` around the weight `healing` currently carries (1.0)
with `healing` itself reduced to compensate, `heal_eff` and `debuffs_removed`
around 0.4; inside the buff profile, `buffed_damage_dealt` and
`protected_damage_taken` around 0.8 replacing most of `buff_value`'s current
0.8. These are shaped to be plausible, not correct.

---

## 6. Interaction with the rest of the engine

- **The premium mechanism is unchanged; the premium values are not.** A premium
  (`poison` 0.45, `tank` 1.10) stays a judgement about how valuable a role is,
  which no amount of device data settles. But four Recon archetypes and two
  Robotics archetypes need six premiums that do not exist yet, and `poison` 0.45
  is now explicitly disputed (§4). Both are decisions, not fits — see §7 item 4.
- **`perf_scale` may need re-fitting.** It sets how many rating points one unit
  of performance is worth at rest. Adding signals changes the spread of the
  performance index, so the resting spread moves with it.
- **`min_baseline_games` (20) applies per stat.** New signals will be
  unscoreable for the first ~20 recorded matches per class and simply drop out,
  which is the correct behaviour and needs no special handling.
- **The reference implementation must be updated in lockstep.**
  `tools/mmr/reseed.py` is the acceptance test for the compiled engine; if it
  does not learn the device signals at the same time, the test silently stops
  testing anything.

---

## 7. Decisions taken, and what is still open

Six of the eight items in the previous draft are now answered. What follows
records the answers so they are not re-litigated, and states plainly what is
left.

### Settled

**1. Recon archetypes — four.** Sniper, bomber, mines, melee, with device sets
in §4. Keyed on **used** devices, not equipped loadout.

**2. Robotics archetypes — two.** Drone and turret/station. *"We'll go with two
for now."* Turrets and stations do not separate; the same players carry both.

**3. Medic buff/clutch discriminant — the offhands.** Buff = protection / power /
frenzy waves over a nanite base; clutch = triage / heal wave / heal grenade with
at most one buff. The `buff_value` floor is replaced.

**5. Taxonomy home — `MatchStats.cpp`.** Recommendation accepted; it already
carries the Group-Heals allowlist and the exclusion list, so recording and
rating share one definition and cannot drift. Still needs doing, but no longer
needs deciding.

**6. Assault beyond roamer and tank.** Deferred by agreement. Nothing to do.

**7. Wave discipline.** `heal_eff` plus `rescues` accepted as *"sharp enough for
now"*, with a specific refinement recorded below.

### Still open — decisions

**4. Premiums for the new archetypes.** Six values that do not exist: sniper,
bomber, mines, melee, drone, turret/station. The existing premiums are
judgements about how valuable a role is, not fitted numbers — poison sits at
0.45 because poison medics win fewer games, tank at 1.10 because holding the
point wins them. The same call is needed here: is a melee Recon doing its job
well worth as much to a team as a sniper doing its job well? The data cannot
answer this and should not be asked to. **The safe default is 1.00 for all
six** — it asserts nothing — and to revisit once each archetype has enough
games to compare win rates, which will take considerably longer than the signal
work.

**Poison medic valuation.** Raised in §4 and repeated here because it is the one
outstanding item that affects a *live* number rather than a future one. Whether
this is fixed structurally (sub-population baselines, rank transform) or by
lowering the premium is open, but the structural fix depends on device data
identifying poison medics, so nothing changes before golive.

### Still open — needs building, not deciding

**Buff-outcome attribution.** The gap identified in the wave-discipline
discussion, and it is a real one:

> "If a frenzy is used to try and save someone on low health but that person
> does no damage, it'll look negatively against the player."

`buffed_damage_dealt` and `protected_damage_taken` credit the *caster* with what
the *target* subsequently did. That is the right instinct — it is what makes
them better than raw `buff_value` — but it means a correct cast on a teammate
who then does nothing scores as a wasted one, and a player who buffs the best
teammate on the server scores well regardless of timing. Frenzy is the worst
case: its whole value is conditional on the target then fighting.

Two mitigations, neither free:

- **Judge the cast, not the outcome** — was the target in a state where the buff
  was the right call (low health for protection, in combat for frenzy)? This
  needs state at cast time, which is a recording change, not a rating change.
- **Normalize by the target's own baseline** — credit `buffed_damage_dealt`
  against what that teammate normally does per minute, so buffing a passive
  player is not punished and buffing a strong one is not automatically rewarded.
  This is computable from existing data with no recording change, and is the
  cheaper of the two to try first.

Flagged as owned by the game side to specify, per *"that's one for me to take
away."* Noted here so the weakness is on the record before any weight is fitted
to those two signals.

**Timing.** When recording goes live, and roughly when a full Sunday of rows
will exist. Affects scheduling only.

### The first thing to check once data exists

**Do amna and Callizle score well on `rescues` and `heal_eff` while remaining
average on raw `healing`?**

That is the whole hypothesis in one test. If yes, the model was under-selling
them, the fix is real, and the same treatment should be extended. If they are
average there too, then either the discipline theory is wrong or it is still not
being measured — and the honest response is to say so rather than keep adding
signals until the answer comes out right.
