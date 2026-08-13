# Device-usage stats → ga-mm integration handoff

For the MMR-rework work on the **`ga-mm` branch**. This doc lives on the
**`device-usage-metrics` branch** — if you are reading from ga-mm, fetch it from
there (`git show device-usage-metrics:docs/claude/device-stats-mmr-handoff.md`).
It is written to be self-contained: everything the perf engine needs to consume
the new data is here, including the traps.

**The problem this solves for ga-mm:** the perf engine's medic signal today is
raw `healing` from `ga_match_player_stats`, which cannot distinguish a
wave-spammer from a rescue medic. The device-usage branch records per-device
effectiveness server-side; this doc says what exists, what it means, and how to
fold it into the per-archetype performance model.

---

## 1. What exists

### 1a. `ga_match_device_stats` — per-device aggregates

Keyed `(instance_id, character_id, task_force, device_id)`. Written by the game
DLL over IPC: every 60s during the match (safety flush), at player leave, and at
mission end. Upserts of absolute totals — resend-safe, survives reconnects
(keyed by character, not connection), uncapped row count.

| Column | Meaning | Trust notes |
|---|---|---|
| `uses` | activations (per hold for continuous fire; boost casts detected via effect application, since morale abilities never enter DeviceFiring) | high for activatables; ≈shots for semi-auto weapons |
| `damage` | post-mitigation damage to enemies | team damage excluded |
| `healing` | HP actually restored **on others** (clamped to missing health) | self-heal and own pets/gear excluded |
| `player_kills` / `bot_kills` | killing blows, device-attributed | |
| `debuffs_removed` | effect groups stripped by this device's cleanses | provenance-gated: the per-impact mitigation bracket, Rest/stealth housekeeping polls, and invuln refcounting can never count. Self-cleanse **counts** (Sealed Systems is self-target by design) |
| `overheal` | heal magnitude clamped away on topped-up targets | `healing + overheal` = total heal output. CAUTION: includes on-hit heal riders (e.g. the Death-Medic backstab heal recorded ~14k overheal on a poison dagger in testing) — filter by device class if using as a medic-discipline signal |
| `power_restored` / `power_wasted` | Power Pool (prop 243) split at the pool cap | Triage Wave is exempt (its +250 rider always overflows the 160-max pool; a designed rescue, not waste) |
| `rescues` | under-25% conditional deliveries: Triage's own conditional group + Group Heal Savior triggers on the delivering group heal | **self-rescues excluded** (own grenade while low) |
| `buffed_damage_dealt` | damage dealt by pawns while under this device's offensive buff (Frenzy Wave) | credited to the buff's caster; the shooter's weapon row is separate — no double counting |
| `protected_damage_taken` | damage taken by pawns under this device's defensive buff (Protection Wave, Protection Boost, Group Heal Savior) | self-protection counts here (unlike `rescues`) — it measures the buff's work, not altruism |
| `boost_targets` | distinct boost applications (reach). `÷ uses` = players caught per cast | Healing Boost currently; extend in `BuffWindowTracking.cpp` / `MatchStats.cpp` tables |
| `boost_overwrites` / `boost_wasted_secs` | times this medic's **live** boost was replaced by another caster (newest-wins), and the lifetime lost | lands on the *victim* medic's row — descriptive, not blame (see §3) |

Joins: `user_id` → `ga_users.id` (**use `ga_users.username` for names** —
`ga_players` is a session table, cleared on restart). Device names:
`asm_data_set_items.item_id = device_id` → `name_msg_id` →
`asm_data_set_msg_translations.message` (note: `item_id`, NOT `ref_device_id`).

### 1b. `ga_match_events` — new event types

All timestamped (`game_time`), same identity columns as existing KILL/DEATH rows.

| event_type | actor | target | owner | device_id | detail | flags |
|---|---|---|---|---|---|---|
| `CLEANSE` | cleanser | cleansed pawn | — | cleansing device | category purged (join `asm_data_set_valid_values.value_id`) | strip count |
| `SAVIOR` | medic | rescued pawn | — | delivering group heal | 852 | — |
| `DEVICE_USED` | user | — | — | device | — | — |
| `BOOST_APPLY` | caster | recipient | — | boost | — | — |
| `BOOST_OVERWRITE` | the overwriter | recipient | **the medic whose ticks were lost** | the wasted boost | seconds remaining | — |

`DEVICE_USED` is allowlisted (Group Heals family from data + the three boosts) so
weapons never flood the table. A cast's `BOOST_APPLY` rows minus its
`BOOST_OVERWRITE` rows = fresh coverage — the "was the second medic's cast
justified" ratio. `SAVIOR` with actor == target is a self-rescue (evented,
excluded from the counter).

---

## 2. Gating — why ga-mm can trust the data

Recording resolves per instance at IPC HELLO, as
`global AND per-queue`:

- master `stats_enabled` — false on home maps (nothing records there, ever)
- global `device_stats_enabled` / `effectiveness_enabled` — `control-server.json`, default true
- per-queue `ga_queues.record_device_stats` / `record_effectiveness` — **default 0; seeded 1 only for `merc` and `1v1`** (one-time seed on column creation; later manual toggles survive restarts)

**Consequence for ga-mm: device stats exist only for competitive queues by
construction.** PvE farming can't reach this data, so no bot-inflation
filtering is needed on the consumption side. (This mirrors the perf engine's
existing choice to read matches, and is stricter.) `queue_id = 0` ad-hoc
instances fall back to the globals — dev-test data; exclude by joining
`ga_instances.queue_id != 0` if it matters.

Also excluded at source: all jetpacks (asm slot 806), Bionics, Regeneration,
Power Stim (+ tier variants) — extension point in `MatchStats.cpp`
(`g_excludedDevices`).

---

## 3. Candidate medic signals for the perf model

The archetype-premium structure in `MmrService` ("signals that don't matter
within a class drop out") is exactly where these belong. Suggested per-player
per-match features, all derivable from `ga_match_device_stats` aggregated over
the player's rows:

| Signal | Formula | What it separates |
|---|---|---|
| effective-heal ratio | `SUM(healing) / NULLIF(SUM(healing + overheal), 0)` over medic-kit devices | discipline vs wave-spam (filter to heal-class devices to dodge the rider-heal caveat) |
| rescues | `SUM(rescues)` | clutch play under pressure; already self-excluded |
| cleanses | `SUM(debuffs_removed)` | counterplay awareness |
| damage enabled | `SUM(buffed_damage_dealt)` | Frenzy timing/targeting |
| damage absorbed | `SUM(protected_damage_taken)` | Protection timing/targeting |
| boost efficiency | reach `SUM(boost_targets)/SUM(uses)`; freshness from events: `(APPLY − OVERWRITE) / APPLY` per cast | placement + inter-medic coordination |
| boost victimhood | `boost_overwrites` / `boost_wasted_secs` | NOT a skill signal for the row's owner — being stomped is the *other* medic's timing. Use as a match-context covariate or ignore; do not penalize |

Aggregation shape (mirrors how the engine already reads
`ga_match_player_stats`):

```sql
SELECT ds.instance_id, ds.user_id, ds.character_id,
       SUM(ds.healing) heal, SUM(ds.overheal) overheal,
       SUM(ds.rescues) rescues, SUM(ds.debuffs_removed) cleansed,
       SUM(ds.buffed_damage_dealt) dmg_enabled,
       SUM(ds.protected_damage_taken) dmg_absorbed,
       SUM(ds.boost_targets) boost_reach, SUM(ds.uses) uses
FROM ga_match_device_stats ds
GROUP BY ds.instance_id, ds.user_id, ds.character_id;
```

`LEFT JOIN` this against the engine's match rows on
`(instance_id, character_id)`; absent rows = zeros (older matches predate
recording — treat as missing, not zero, when normalizing).

---

## 4. Caveats the model must respect

1. **Neutral counters, judgment in events.** `boost_overwrites` on A's row is a
   fact about what happened TO A. The fairness metric for B lives in the event
   stream (fresh-coverage ratio). Do not turn either into a naive penalty.
2. **Waves never affect their caster** (game rule) — a solo-queue medic's heal
   numbers depend on having teammates near; normalize by match context.
3. **`uses` semantics vary by device class** — activations for waves/boosts,
   ≈shots for weapons. Only ratio within a device, never across classes.
4. **Cleanse misses are invisible for waves** (the game pre-filters
   non-matching remove-groups upstream — measured), but visible for
   self-cleansers. "Cast but nothing to cure" needs `DEVICE_USED` joins, not
   counters.
5. **Triage vs Savior**: Triage never procs Group Heal Savior (measured,
   single-cast tests). Triage rescues arrive via its own conditional;
   wave/grenade rescues via SAVIOR. `rescues` already unifies both — don't
   double-count by also counting SAVIOR events.
6. **Rows can lag up to 60s** mid-match (flush cadence); events are immediate.
   Final totals land at mission end — the engine's post-match consumption
   pattern is unaffected.
7. Skill-sourced oddities recorded faithfully but worth knowing: Combat
   Off-Hand Utility buffs the *target* of Area Poisons (confirmed live, design
   intent unknown); the Death-Medic backstab heal rider lands on the enemy and
   shows as overheal on daggers.

---

## 5. Where the code is

Branch `device-usage-metrics` (fork), all pushed. Recording:
`src/GameServer/Stats/MatchStats.{hpp,cpp}` (entry points, registries,
exclusions, toggles), `src/GameServer/Stats/DeviceStats.cpp` (baseline mirror),
`src/GameServer/TgGame/_effect_core/{CleanseTracking,BuffWindowTracking,EffectCredit}`
(provenance + windows), hook sites in `TgEffect__TrackStats.cpp` /
`TgEffect__CheckEffectBuffModifier.cpp` /
`TgEffectManager__RemoveEffectGroupsByCategory.cpp`. Control-server side:
`IpcServer.cpp` (HELLO_ACK toggles, MATCH_DEVICE_STATS handler),
`Database.{hpp,cpp}` (schema, upsert, queue toggles). Protocol:
`src/Shared/IpcProtocol.hpp`.

Verification record: instances 202–213 in the dev DB, plus
`docs/claude/effective-device-use.md` (measured corrections flagged inline).
