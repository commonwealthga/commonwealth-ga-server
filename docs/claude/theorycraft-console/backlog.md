# Backlog

Shared to-do list for the `theorycraft-console` work. Items have stable ids so they can be
referenced in conversation ("let's do C2 next"). Companion to
[project-status.md](project-status.md), which describes what is already settled.

**Status key:** `open` · `blocked` (needs an in-game test or external answer) · `wip` · `done`

---

## G — The goal

The thing all of this is for. Everything else is either a prerequisite or a convenience.

| id | item | status | notes |
|---|---|---|---|
| **G1** | **Two-sided combat resolve** | **done** | Attacker build + defender build → actual damage dealt. This is the original ask: *"this medic uses Frenzy on this recon, so when he shoots this assault he does X"*. |
| ~~G1.1~~ | Defender spec through `CalcProtection` | **done** | `GA.statsFor(ctx)` aggregates any build without disturbing the displayed one; `GA.mitigate` implements the three axes exactly as decompiled, including the integer floor, immunity at protection ≥ rating, and the half-up rounding. Validated to the unit against the Ballista A/B and the incendiary-grenade test (explosion 538/696, DoT 136/70). |
| ~~G1.2~~ | Apply prop 316 "Additional Damage Taken" **before** mitigation | **done** | Applied ahead of the axes in `GA.mitigate`. |
| ~~G1.3~~ | Per-hit health cap (anti-one-shot) | **done** | In `GA.mitigate` behind `healthCapArmed`; players only, bots exempt, floor `ceil(maxHP × 10%)`. |
| ~~G1.4~~ | Third-party effects — a medic buffing/debuffing either side | **done** | Driven by the effect-group type rather than by special cases: groups aimed at someone else (264 HIT · 272 HIT_IN_AIR · 505 HIT_SITUATIONAL · 398 BLOCK_HIT) are extracted as a device's *projected* effects and routed to a recipient. Each projecting device gets a self / other-side control, and an "in flight" strip shows everything crossing between the builds. **Limitation:** single-pass — a buff on the thrower does not re-scale what it throws. |
| ~~G1.5~~ | UI for two combatants | **done** | Delivered as the **Combat** tab: a build picker per side, swap, per-side gear toggles, and a shot list with the mitigation breakdown per axis. Two full build panes side by side were rejected as unreadable. |

---

## C — Correctness (known-wrong or unverified)

Ordered by how much they distort results.

| id | item | status | notes |
|---|---|---|---|
| ~~C1~~ | Output Mod re-applied at hit time? | **done** | **Settled by test:** applied once, but as its **own multiplicative layer** — it does not sum with the other rolled mods. `base × (1+Output) × (1+other item) × (1+skills)`. Matched to the unit on both shots (867 / 991). The old summed model was ~8% low on every damage figure. §21. |
| ~~C2~~ | Turret deploy-time units | **done** | `prop 279` is build work in seconds, divided by `(1 + DeployRate)`. Only deploy figure in the data; coherent scale from bombs 0.001 to big turrets 40. Turrets/drones are bots, stations/walls are deployables — both use 279. Folded into the console for every deployable, with the arm acceleration shown. Residual: the data has two turret tiers (25/40), not four. §22. |
| ~~C3~~ | Killer Instinct self-shot leak | **answered** | **Confirmed a server bug, and a fix is due.** The triggering shot lands 50 damage harder than it should; the Ballista's type-264 debuff leaves shot 1 clean. **Mechanism now settled:** `CalcProtection` integer-floors protection, so the apparent 26.875 is unreachable — it is the *full* −10 applied to ~31% of the shot, not a fraction of the debuff applied to all of it. Look for damage submitted in more than one piece. Until the fix ships the console models it as it currently behaves, behind one named flag. Fix handoff written up in [issues/killer-instinct-diag.md](issues/killer-instinct-diag.md) — self-contained, for a chat on the server repo. §damage-pipeline "SECONDARY QUIRK". |
| ~~C4~~ | Power regen during drain | **done** | **Confirmed in game:** passive regen does not run while power is being consumed. The time-to-empty figure is the full sustain; wording updated from an assumption to a statement. |
| ~~C5~~ | Dome Shield Boost has no numeric effects | **done** | It is a *deployable shield*, same mechanic as Force Wall — both spawn a force field whose substance is the deployable's **health** (2500 / 2370) plus Persist Time, neither of which was being read. Now surfaces Structure HP (scaled by the Robotics deployable-health skills) and Duration. Also fixed Medical Station, Power Station and Sensor. |
| ~~C6~~ | Regeneration vs Healing Grenade | **closed** | No conflict, and none was ever claimed — my note misattributed it. They sit in different categories (1341 Self Heal vs 772 Regeneration) and stack. Only the Nanite line contends with Healing Grenade. |
| ~~C7~~ | Dual Daggers alt-fire | **closed** | Confirmed: the alt fire **is** the block, and it counts as the weapon's second mode. The console's off / primary / block cycle is correct. |
| ~~C8~~ | Narrow-viewport layout | **closed** | Desktop-only by decision. Verified clean 980–1600px; phone breakpoints are out of scope. |

---

## D — Data / model gaps

| id | item | status | notes |
|---|---|---|---|
| D1 | `calc method 119` semantics | open | Named "N/A" in the enum; behaves as a *set* rather than arithmetic. Used on 195 effects, mostly sensor config. |
| D2 | Sensor Visibility Config bit meanings | open | 34 stealth / 35 through-walls / 36 low-health map to observed behaviour, but the encoding is unknown. |
| D3 | Robotics Sensor reveal parameters | open | Detection lives on the spawned entity, so its range/FOV are not surfaced on the device. Same likely applies to other deployables. |
| D4 | `effect_groups.health = 1` on melee weapons | open | Category 304 Slow. The field means "shield pool" elsewhere; here it means something else. Currently excluded, but unexplained. |
| D6 | Effects linger across a weapon swap | open | Confirmed mechanic: a Nanite gun's effect keeps running after the medic swaps to another weapon. The console models "which weapon is out", not "what is still ticking from the last one". Relevant once F4 (DPS/TTK) exists, since it changes what a rotation actually delivers. |
| ~~D5~~ | Conditional buffs not yet modelled as third-party | **done** | Resolved with G1.4 — projection is by effect-group type, so anything aimed at another actor is carried automatically rather than device by device. |

---

## F — Console features

| id | item | status | notes |
|---|---|---|---|
| ~~F1~~ | Equipment / armour swapping | **done** | Delivered as the **TheoryCrafter** tab: a blank build where you pick a class, fill the eight slots from that class's pool, choose each item's mod roll, and spend skill points. Shares the resolver, armour panel, trees and My Player with the character view. Armour swapping still uses the preset configs rather than per-piece rolls. |
| F2 | Save / share a build | open | Export a build (skills + gear + armour + active toggles) as JSON or a URL fragment, so builds can be compared or posted. |
| F3 | Compare two builds side by side | open | Natural precursor to G1 and useful on its own. |
| F4 | DPS / time-to-kill | open | Damage ÷ refire, bounded by power pool and cooldowns. C1 and C4 are now settled, so this is unblocked. Needs D6 (swap-lingering) to be honest about rotations. |
| F5 | Search / filter on the loadout page | open | 115 devices is a lot to scroll. Filter by class, category, or effect type. |
| F6 | Mini skill trees on loadout cards | open | Built, then disabled for performance (~4,400 positioned nodes with inline images). Could return via sprite sheet or render-on-expand. |
| F7 | Show *available* vs allocated skills per device | open | Tiles list allocated skills that affect them; showing the unallocated ones too would help planning. |

---

## T — Tooling / hygiene

| id | item | status | notes |
|---|---|---|---|
| T1 | Generators hard-code `E:\GA_LOCAL\gaa.db` and icon paths | open | Fine for one machine; would need parameterising before anyone else can rebuild. |
| T2 | Hand-rolled PNG downscaler | open | No Pillow available, so `gen3.py` uses a zlib-based decoder/encoder. Works, but Pillow would be simpler if it can be installed. |
| T3 | Retired icon collector | done | Moved to `tools/device-console/retired/`, kept for repeatability. |

---

## Suggested order

1. ~~C1~~ **done** — the offensive chain is now settled and matches measurement to the unit.
2. ~~C2~~ **done** — folded into the console.
3. ~~F1~~ **done** — the TheoryCrafter tab.
4. **G1.1–G1.3** — the mitigation stage. The single biggest step toward the point of all this.
5. **G1.4/D5**, then **G1.5**.

**The whole C section is now resolved** — C1 through C8. Nothing in the correctness list is
blocked or unverified.

**G1 is done** — G1.1 through G1.5. The console resolves a real fight between two saved builds.

What is left is refinement rather than foundation:

1. **Shot sequences** — shot 1 vs shot 2 with on-hit debuffs and self-buffs live. Every in-game
   measurement so far has been a two-shot test, so this is what makes the console directly
   checkable against them. Needs D6.
2. **F4 DPS / time-to-kill**, now unblocked by C1 and C4.
3. **F2 / F3** — save a build, then compare two.
