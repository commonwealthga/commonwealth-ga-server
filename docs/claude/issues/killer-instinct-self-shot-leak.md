**🐛 Killer Instinct debuff partially applies to its own triggering shot**
*(type-505 "Hit-Situational" apply ordering)*

**Summary**
The Recon skill **Killer Instinct** (effect group `16596`, type 505 "Hit-Situational", prop 155 Protection-Physical −10, gated on target HP > 75%) applies a *fraction* of its protection debuff to the very shot that triggers it. On-hit debuffs should only affect *subsequent* shots — the triggering shot should be mitigated against full protection.

**Test conditions**
- **Weapon:** Ballista OC
- **Attacker build:** full Marksman setup — Recon Rifle Range, Power Cost, Damage, Effective Range, Accuracy + **Eagle Eye, Killer Instinct, Sureshot, Super Sharpshooter**
- **Target:** live player, **no armour, no skills** (base protections only — Physical protection = 30), at **full HP** before shot 1 of every run
- **Range:** point-blank (melee range), same spot each run, shot 2 fired within the debuff windows (KI 3s / Ballista 5s)
- **Reset methodology:** when removing a skill (Killer Instinct / Eagle Eye) for a run, the freed point went into a **random Balanced-tree node — never into "Ranged Damage"**, so the offensive damage stayed constant and only the defensive side varied

**Data** (mitigation ≈ protection, 1:1; base Physical protection 30):
```
Build              Shot 1 (dealt / mit)   Shot 2 (dealt / mit)
Full build          1170 / 430             1548 / 116
No Eagle Eye        1170 / 430             1498 / 166
No Killer Instinct  1120 / 480             1381 / 283
```

**The finding:** shot 1 is the control (debuffs apply *after* it). Yet **with Killer Instinct, shot-1 mitigation is 430 (protection ≈ 26.9); without it, shot-1 mitigation is 480 (protection = 30, clean)**. So KI shaves ~3.1 protection points off its own triggering shot. The Ballista's own on-hit debuff (type 264 "Hit", eg `18975`) does **not** do this — its triggering shot stays clean — so the behaviour is specific to the **type-505** dispatch.

**Expected**
Shot 1 mitigated against full protection (30); the −10 debuff should only affect shots 2+.

**Suspected area**
Apply ordering between type-505 (Hit-Situational) and type-264 (Hit) effects in `TgDeviceFire.SubmitHitEffects` (UnrealScript). The leak is *partial* (~3.1 of the −10 = exactly 50 damage on shot 1), suggesting the triggering shot's damage is computed in stages with only part seeing the reduced protection — worth confirming against original-server behaviour to classify bug-vs-faithful.

**Impact**
Minor — ~4% extra damage on the first shot only, favouring the attacker.
