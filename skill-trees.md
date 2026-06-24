# Skill Trees

Human-readable reference for authoring opening skill builds in
[`src/ControlServer/Loadouts/OpeningSkills.cpp`](src/ControlServer/Loadouts/OpeningSkills.cpp).

Each skill node is a `(skill_group_id, skill_id)` pair, allocated binary
(`points = 1`). Names below were transcribed from the in-game skill screen (the
DB ships them unlinked — see `tools/skill_catalog.sql` for the effect-based
lookup that backs the **Effect** column here).

## How it maps

- **Group 155 is the common tree** every class shares (labelled **Balanced**
  here). Each class then has **two class-specific trees**.
- **Budget: 13 nodes per loadout profile** (item_profile_id 1..5). Pick any 13
  across the available trees.
- Authoring format — paste `(group, skill)` into the profile slot in
  `OpeningSkills.cpp`, name in the trailing comment:

  ```cpp
  { 155, 533 },  // Passive Protection
  { 156, 703 },  // Super Healer
  ```

| Class    | profile_id | Common (Balanced) | Class trees                 |
|----------|-----------:|-------------------|-----------------------------|
| Medic    | 567        | 155               | 156 Healer · 157 Poison     |
| Robotics | 679        | 155               | 162 · 163 *(TODO)*          |
| Assault  | 680        | 155               | 158 · 159 *(TODO)*          |
| Recon    | 681        | 155               | 160 · 161 *(TODO)*          |

---

## Medic (567)

### Balanced — group 155 *(common tree, shared by all classes)*

| skill_id | Name                      | Effect(s)                                                                                                     |
|---------:|---------------------------|--------------------------------------------------------------------------------------------------------------|
| 533      | Passive Protection        | `[g]` Protection - Physical 4                                                                                 |
| 676      | Advanced Passive Protection | `[g]` Protection - Physical 5                                                                              |
| 526      | Jetpack Power             | `[p]` Power Pool Cost 30                                                                                      |
| 519      | Power Pool Increase       | `[p]` Power Pool Max 0.4                                                                                      |
| 550      | Team Boost Increase       | `[m]` Required Morale Points Modifier 25                                                                      |
| 545      | Health                    | `[n]` Health Max Modifier 10                                                                                  |
| 524      | Power Pool Return         | Power Pool Recharge Rate 0.2                                                                                  |
| 527      | Damage Increase: AoE      | `[d]` Damage Modifier - AoE 6                                                                                 |
| 528      | Damage Increase: Ranged   | `[d]` Damage Modifier - Range 6                                                                               |
| 680      | Damage Increase: Melee    | `[d]` Damage Modifier - Melee 6                                                                               |
| 525      | Offhand Recharge          | `[c]` Recharge Time Modifier 10                                                                               |
| 677      | Super Agent               | `[g]` Prot-Physical 4, `[c]` Recharge 5, `[d]` Dmg Melee/Range/AoE 5, `[h]` Healing 5, `[n]` Health Max 10 |

### Healer — group 156

| skill_id | Name              | Effect(s)                                                                                              |
|---------:|-------------------|--------------------------------------------------------------------------------------------------------|
| 746      | Group Heal Size   | `[x]` AOE Radius Modifier 15                                                                            |
| 747      | Group Heal Effect | `[h]` Effect Healing Modifier 8                                                                         |
| 651      | Beam Heal Cost    | `[p]` Power Pool Cost 10                                                                                |
| 652      | Beam Heal Boost   | `[h]` Effect Healing Modifier 4                                                                         |
| 801      | Beam Heal Boost II | `[h]` Effect Healing Modifier 6                                                                        |
| 804      | Heal Durations    | `[t]` Effect Lifetime Modifier 12                                                                      |
| 805      | Heal Durations II | `[t]` Effect Lifetime Modifier 18                                                                      |
| 902      | Fast Regeneration | Health 80                                                                                              |
| 903      | Buff Enhancement  | `[*]` Effect Potency Modifier 40                                                                       |
| 852      | Group Heal Savoir | GroundSpeed 0.1, `[g]` Protection - Physical 10                                                        |
| 904      | Clutch Healer     | `[c]` Recharge Time Modifier 20                                                                        |
| 753      | Passive Healer    | `[t]` Lifetime 10, `[h]` Healing 5, `[n]` Health Max 10                                                |
| 703      | Super Healer      | Health 50, GroundSpeed 0.1, `[r]` Prot-Ranged 8, `[b]` Prot-AOE 8, `[h]` Healing 10, `[m]` Morale 10  |

### Poison — group 157

| skill_id | Name                  | Effect(s)                                                                                             |
|---------:|-----------------------|-------------------------------------------------------------------------------------------------------|
| 891      | Medic Melee I         | `[d]` Damage Modifier - Melee 3, `[m]` Protection - Melee 10                                           |
| 892      | Medic Melee II        | `[d]` Dmg Melee 4, `[m]` Prot-Melee 7, Power Pool Cost - Block (per sec) 50                            |
| 893      | Medic Melee III       | `[d]` Damage Modifier - Melee 5, `[m]` Protection - Melee 6                                            |
| 807      | Combat Off-Hand Power | `[d]` Damage Modifier - AoE 10                                                                         |
| 667      | Poison Duration       | `[t]` Effect Lifetime Modifier 15                                                                      |
| 812      | Poison Duration II    | `[t]` Effect Lifetime Modifier 25                                                                      |
| 674      | Bio Rifle Range       | `[q]` Device Effective Range 25, `[r]` Device Range 50                                                 |
| 760      | Bio Rifle Power       | `[p]` Power Pool Cost 10                                                                               |
| 675      | Bio Rifle Damage      | `[d]` Damage Modifier - Range 10, `[*]` Effect Potency Modifier 20                                     |
| 806      | Combat Off-Hand Utility | `[x]` AOE Radius 10, GroundSpeed 0.1, `[g]` Protection - Physical 5                                  |
| 811      | Quick Poisons         | `[d]` Damage Modifier - AoE 5, `[c]` Recharge Time Modifier 15                                         |
| 810      | Battle Medic          | GroundSpeed 0.1, `[m]` Prot-Melee 4, `[r]` Prot-Ranged 4, `[y]` Effect Heal (Self) 6                  |
| 814      | Fighting Medic        | `[d]` Damage Modifier - AoE 8, `[n]` Health Max Modifier 10                                            |
| 815      | Death Medic           | Health 60, `[*]` Potency 50, `[*]` Potency 200, `[c]` Recharge 5, Remote Activation Time 20            |

---

> For the classes below the **Name** column is blank — fill from the in-game
> skill screen. The common **Balanced** tree (155) is shared; see Medic above.

## Robotics (679)

*Common tree: 155 (Balanced, above). Class trees: 162 / 163.*

### Group 162 — *(tree name TBD)*

| skill_id | Name | Effect(s)                                                                                 |
|---------:|------|-------------------------------------------------------------------------------------------|
| 612      |      | `[y]` Pet Deploy Time 15, `[p]` DeployRate Buff 20                                         |
| 613      |      | `[p]` DeployRate Buff 20, `[d]` Buff - Damage Buff 10                                      |
| 714      |      | `[c]` Recharge 15, `[c]` Recharge 10, `[g]` Prot-Physical 5, `[d]` Pet Dmg 5, `[v]` Pet Max HP 10, `[h]` Healing 20 |
| 785      |      | `[p]` Power Pool Cost 10, `[n]` Health Max Deployables 10, `[v]` Pet Max HP 10             |
| 786      |      | `[h]` Healing 10, `[n]` Health Max Deployables 10, `[v]` Pet Max HP 10                     |
| 793      |      | `[x]` AOE Radius 20                                                                        |
| 795      |      | `[n]` Health Max Deployables 10, `[*]` Potency 20, `[*]` Potency 50                        |
| 845      |      | `[d]` Pet Dmg 5, `[r]` Pet Range 10                                                        |
| 851      |      | `[n]` Health Max 10, `[y]` Pet Deploy Time 20, `[*]` Potency 15, `[*]` Potency 50          |
| 885      |      | `[d]` Dmg-Melee 3, `[m]` Prot-Melee 10                                                     |
| 886      |      | `[d]` Dmg-Melee 4, `[m]` Prot-Melee 7, Power Pool Cost - Block (per sec) 50                |
| 887      |      | `[d]` Dmg-Melee 5, `[m]` Prot-Melee 6                                                      |
| 907      |      | `[c]` Recharge 10, `[m]` Required Morale Points 10                                         |

### Group 163 — *(tree name TBD)*

| skill_id | Name | Effect(s)                                                                                 |
|---------:|------|-------------------------------------------------------------------------------------------|
| 630      |      | `[v]` Pet Max HP 15                                                                        |
| 631      |      | `[c]` Recharge 8, `[l]` Pet LifeSpan 10                                                    |
| 672      |      | `[r]` Device Range 50                                                                      |
| 673      |      | `[d]` Dmg-Range 10, `[d]` Dmg-AoE 10                                                       |
| 789      |      | `[d]` Pet Dmg 10                                                                           |
| 790      |      | `[v]` Pet Max HP 10, `[n]` Health Max 10                                                   |
| 791      |      | `[r]` Prot-Ranged 8, `[b]` Prot-AOE 8                                                      |
| 796      |      | `[p]` Power Pool Cost 10                                                                   |
| 797      |      | `[d]` Pet Dmg 5, `[c]` Recharge 12, `[l]` Pet LifeSpan 15                                  |
| 798      |      | GroundSpeed 0.1, `[d]` Dmg-Range 5, `[m]` Prot-Melee 5, `[r]` Prot-Ranged 5, `[b]` Prot-AOE 5, `[d]` Dmg-AoE 5, `[d]` Pet Dmg 10, `[l]` Pet LifeSpan 20 |
| 864      |      | `[r]` Pet Range 15                                                                         |
| 905      |      | Pet Accuracy 5, `[r]` Pet Range 10, `[r]` Pet Dmg Radius 20                                |
| 906      |      | GroundSpeed 0.05, AirSpeed 0.1, Protection - Slow 15, Protection - Knockback 15            |

## Assault (680)

*Common tree: 155 (Balanced, above). Class trees: 158 / 159.*

### Group 158 — *(tree name TBD)*

| skill_id | Name | Effect(s)                                                                                 |
|---------:|------|-------------------------------------------------------------------------------------------|
| 532      |      | `[c]` Recharge 15                                                                          |
| 546      |      | `[T]` Threat 15, `[s]` GroundSpeed 100, `[r]` Prot-Ranged 5, `[b]` Prot-AOE 5, `[n]` Health Max 30 |
| 762      |      | `[s]` Effect Shield 25, `[t]` Effect Lifetime 40                                           |
| 767      |      | `[y]` Heal (Self) 7, `[m]` Prot-Melee 10, `[r]` Prot-Ranged 10, `[b]` Prot-AOE 10          |
| 768      |      | `[b]` Prot-AOE 15                                                                          |
| 769      |      | `[r]` Prot-Ranged 15                                                                       |
| 859      |      | `[n]` Health Max 15                                                                        |
| 860      |      | `[r]` Prot-Ranged 4, `[b]` Prot-AOE 4, `[n]` Health Max 10, Prot-Stun 15, Prot-Knockback 15 |
| 888      |      | `[d]` Dmg-Melee 3, `[m]` Prot-Melee 10                                                     |
| 889      |      | `[d]` Dmg-Melee 4, `[m]` Prot-Melee 7, Power Pool Cost - Block (per sec) 50                |
| 890      |      | `[T]` Threat 50, `[d]` Dmg-Melee 5, `[m]` Prot-Melee 6                                     |
| 912      |      | `[m]` Required Morale Points 10, `[t]` Effect Lifetime 30                                  |
| 913      |      | `[g]` Prot-Physical 25                                                                     |

### Group 159 — *(tree name TBD)*

| skill_id | Name | Effect(s)                                                                                 |
|---------:|------|-------------------------------------------------------------------------------------------|
| 681      |      | `[x]` AOE Radius 10, `[q]` Device Effective Range 20                                       |
| 682      |      | `[*]` Potency 100, `[a]` Accuracy 4, `[d]` Dmg-Range 6, `[d]` Dmg-AoE 6, `[q]` Device Eff Range 20, `[x]` AOE Radius 20, `[d]` Pet Dmg 6, `[r]` Pet Dmg Radius 20 |
| 710      |      | `[d]` Dmg-AoE 10                                                                           |
| 711      |      | `[p]` Power Pool Cost 10                                                                   |
| 771      |      | `[r]` Device Range 50                                                                      |
| 772      |      | `[p]` Power Pool Cost 10                                                                   |
| 773      |      | `[d]` Dmg-Range 10                                                                         |
| 806      |      | `[x]` AOE Radius 10, `[r]` Pet Dmg Radius 10, `[q]` Device Effective Range 20              |
| 807      |      | `[d]` Dmg-AoE 10, `[d]` Pet Dmg 10, `[t]` Effect Lifetime 25                               |
| 820      |      | `[c]` Recharge 15                                                                          |
| 823      |      | Power Pool Recharge Rate 0.1, `[p]` Power Pool Max 0.2, `[p]` Power Pool Cost 5            |
| 910      |      | `[*]` Potency 50                                                                           |
| 911      |      | `[t]` Effect Lifetime 30                                                                   |

## Recon (681)

*Common tree: 155 (Balanced, above). Class trees: 160 / 161.*

### Group 160 — *(tree name TBD)*

| skill_id | Name | Effect(s)                                                                                 |
|---------:|------|-------------------------------------------------------------------------------------------|
| 581      |      | MakeVisible Fade Rate 0.5, `[p]` Power Pool Cost 30                                        |
| 584      |      | `[d]` Dmg-AoE 10, `[t]` Effect Lifetime 25, `[*]` Potency 25                               |
| 599      |      | `[g]` Prot-Physical 1, `[g]` Prot-Physical 8                                               |
| 692      |      | GroundSpeed 0.15, `[c]` Recharge 10, Remote Activation Time 25, `[y]` Pet Deploy Time 25, `[d]` Dmg-Melee 6, `[d]` Dmg-AoE 10 |
| 742      |      | GroundSpeed 0.1, `[f]` Falling Damage 1                                                    |
| 825      |      | `[m]` Prot-Melee 10, `[b]` Prot-AOE 10                                                     |
| 826      |      | `[x]` AOE Radius 10, `[q]` Device Effective Range 20                                       |
| 827      |      | `[d]` Dmg-AoE 15                                                                           |
| 828      |      | Remote Activation Time 10, `[c]` Recharge 15, `[y]` Pet Deploy Time 20                     |
| 837      |      | `[t]` Effect Lifetime 30, `[l]` Pet LifeSpan 30                                            |
| 894      |      | `[d]` Dmg-Melee 3, `[m]` Prot-Melee 10                                                     |
| 895      |      | `[d]` Dmg-Melee 4, `[m]` Prot-Melee 7, Power Pool Cost - Block (per sec) 50                |
| 896      |      | `[d]` Dmg-Melee 5, `[m]` Prot-Melee 6                                                      |

### Group 161 — *(tree name TBD)*

| skill_id | Name | Effect(s)                                                                                 |
|---------:|------|-------------------------------------------------------------------------------------------|
| 577      |      | `[t]` Effect Lifetime 30                                                                   |
| 597      |      | `[r]` Device Range 50                                                                      |
| 598      |      | `[d]` Dmg-Range 10, `[T]` Threat 10                                                        |
| 690      |      | Accuracy Correction Rate 30                                                                |
| 693      |      | `[d]` Dmg-Range 5, Attack Rate - Ranged 5, `[d]` Dmg-Range 10, Accuracy Correction Rate 20 |
| 784      |      | `[p]` Power Pool Cost 10                                                                   |
| 831      |      | `[*]` Potency 50                                                                           |
| 832      |      | `[c]` Recharge 15                                                                          |
| 834      |      | `[t]` Effect Lifetime 20, `[*]` Potency 30, `[*]` Potency 50                               |
| 835      |      | `[r]` Device Range 30, `[q]` Device Effective Range 30                                     |
| 836      |      | `[g]` Prot-Physical 10                                                                     |
| 908      |      | AirSpeed 0.15                                                                              |
| 914      |      | `[d]` Dmg-Range 4, Projectile Speed 30                                                     |
