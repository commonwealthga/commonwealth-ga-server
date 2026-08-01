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
| **G1** | **Two-sided combat resolve** | open | Attacker build + defender build → actual damage dealt. This is the original ask: *"this medic uses Frenzy on this recon, so when he shoots this assault he does X"*. |
| G1.1 | Defender spec through `CalcProtection` | open | Three axes (category / damage-type / attack-type), multiplied. Formula in damage-pipeline §8. `GA.resolve` already takes a spec per side. |
| G1.2 | Apply prop 316 "Additional Damage Taken" **before** mitigation | open | §10. Ordering matters. |
| G1.3 | Per-hit health cap (anti-one-shot) | open | §8. `s_bApplyHealthCap`. |
| G1.4 | Third-party effects — a medic buffing/debuffing either side | open | `activeState()` already returns "something is affecting this player + the source of each part", which is deliberately the shape a second actor produces. |
| G1.5 | UI for two combatants | open | Drag-drop interaction was the original sketch. Needs a design pass once G1.1–G1.4 exist. |

---

## C — Correctness (known-wrong or unverified)

Ordered by how much they distort results.

| id | item | status | notes |
|---|---|---|---|
| **C1** | Output Mod re-applied at hit time? | blocked | Currently counted **once**, in the item layer. If the engine re-applies it, every damage figure is low. Worth ~60 on a Ballista. Needs a controlled in-game test. §6. |
| **C2** | Turret deploy-time units | blocked | `prop 279 ÷ (1 + DeployRate)` gives 25 → 4.55s with an arm, but turrets deploy in 1–2s in game. Mechanic and arm linkage confirmed; the base scale is not. |
| **C3** | Killer Instinct self-shot leak | blocked | ~3-point discrepancy. Raised as a Discord issue. Needs a log capture or original-server reference. |
| **C4** | Power regen during drain | blocked | Time-to-empty assumes passive regen is suspended while draining. Untested — if regen continues, block durations are understated. |
| **C5** | Dome Shield Boost has no numeric effects | open | The only device of 115 with zero numeric chips. Cause unknown. |
| **C6** | Regeneration vs Healing Grenade | blocked | You expected these to conflict; the data says different categories (1341 Self Heal vs 772 Regeneration) so they stack. Only the Nanite line contends with Healing Grenade. Worth an in-game check. |
| **C7** | Dual Daggers alt-fire | blocked | `right_click_behavior = 894` (block) and one mode, so the console offers off/primary/block. You believed it has an alt fire — confirm what RMB actually does. |
| **C8** | Narrow-viewport layout unverified | open | Clean at 980–1600px, but the browser pane will not go below 980 so phone breakpoints are untested rather than confirmed. |

---

## D — Data / model gaps

| id | item | status | notes |
|---|---|---|---|
| D1 | `calc method 119` semantics | open | Named "N/A" in the enum; behaves as a *set* rather than arithmetic. Used on 195 effects, mostly sensor config. |
| D2 | Sensor Visibility Config bit meanings | open | 34 stealth / 35 through-walls / 36 low-health map to observed behaviour, but the encoding is unknown. |
| D3 | Robotics Sensor reveal parameters | open | Detection lives on the spawned entity, so its range/FOV are not surfaced on the device. Same likely applies to other deployables. |
| D4 | `effect_groups.health = 1` on melee weapons | open | Category 304 Slow. The field means "shield pool" elsewhere; here it means something else. Currently excluded, but unexplained. |
| D5 | Conditional buffs not yet modelled as third-party | open | Protection Wave, Frenzy, Boost Beam etc. exist in the data but are only modelled when *you* carry them. Blocks G1.4. |

---

## F — Console features

| id | item | status | notes |
|---|---|---|---|
| F1 | Equipment / armour **swapping** | open | Gear is currently imported and displayed as equipped. The resolver already accepts an arbitrary roll, so this is mostly UI: pick a different device per slot from the class pool. |
| F2 | Save / share a build | open | Export a build (skills + gear + armour + active toggles) as JSON or a URL fragment, so builds can be compared or posted. |
| F3 | Compare two builds side by side | open | Natural precursor to G1 and useful on its own. |
| F4 | DPS / time-to-kill | open | Damage ÷ refire, bounded by power pool and cooldowns. Depends on C1 and C4 being settled to be trustworthy. |
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

1. **C1** — it silently scales every damage number, so settle it before building anything on top.
2. **C5**, **C2** — small, self-contained, and C5 is a visible hole.
3. **F1** — makes the tool genuinely exploratory rather than a viewer.
4. **G1.1–G1.3** — the mitigation stage. The single biggest step toward the point of all this.
5. **G1.4/D5**, then **G1.5**.

C3, C4, C6, C7 are all `blocked` on in-game observation — worth batching into one test session.
