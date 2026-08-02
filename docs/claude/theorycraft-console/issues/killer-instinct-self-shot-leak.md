**✅ FIXED — Killer Instinct debuff applied to its own triggering shot**
*(type-505 "Hit-Situational" apply ordering)*

**Fixed 2026-08-02**, server-side, branch `killer-instinct-self-shot`. Engineering detail in
[killer-instinct-diag.md](killer-instinct-diag.md).

**Summary**
The Recon skill **Killer Instinct** (effect group `16596`, type 505 "Hit-Situational", prop 155
Protection-Physical −10, gated on target HP > 75%) applied its **full** protection debuff to the
very shot that triggered it. On-hit debuffs should only affect *subsequent* shots — the triggering
shot should be mitigated against full protection.

**Test conditions**
- **Weapon:** Ballista OC (original report); **Scorpia OC** for the fix verification — same base
  damage, damage type, attack type, rating and skill gate, but its on-hit debuff is prop 210
  (self-heal) rather than prop 155, so mitigation has exactly one variable
- **Attacker build:** full Marksman setup — Recon Rifle Range, Power Cost, Damage, Effective Range,
  Accuracy + **Eagle Eye, Killer Instinct, Sureshot, Super Sharpshooter**
- **Target:** live player, no protection mods (base Physical protection = 30), at **full HP** before
  shot 1 of every run
- **Range:** point-blank, same spot each run, shot 2 fired within the debuff windows (KI 3s /
  Ballista 5s)
- **Reset methodology:** removing Killer Instinct drops the point rather than reallocating it, so
  offence is provably constant — confirmed by identical shot-1 raws

**Original data** (Ballista OC, bare target):
```
Build              Shot 1 (dealt / mit)   Shot 2 (dealt / mit)
Full build          1170 / 430             1548 / 116
No Eagle Eye        1170 / 430             1498 / 166
No Killer Instinct  1120 / 480             1381 / 283
```

**The finding, as originally read:** shot 1 is the control, yet with Killer Instinct it dealt 50
more. Reading 1170 as a mitigation outcome implies protection 26.9, so the report concluded the
debuff was leaking *partially* — about 3.1 of its 10 points.

**What it actually was:** the leak was the **whole −10**, and 1170 was not a mitigation figure at
all. It is the anti-one-shot health cap (`Health − ceil(0.1 × maxHealth)` = 1300 − 130), which arms
only at exactly full HP — the condition shot 1 of every run guarantees. The true shot-1 damage was
1600 × 0.80 = 1280, clamped to 1170. The cap also inflates the reported "mitigated" column, since
that figure is computed after the clamp.

**After the fix** (Scorpia OC, raw 1536 shot 1 / 1598 shots 2–3):
```
Build              Shot 1 (dealt / mit)   Shots 2-3 (dealt / mit)
Killer Instinct     1075 / 461  prot 30    1278 / 320  prot 20
No Killer Instinct  1075 / 461  prot 30    1119 / 479  prot 30
```

Shot 1 is now identical across builds and mitigated at full protection 30. The −10 still lands in
full on shots 2–3, and Killer Instinct still reads −10 rather than −13 with Eagle Eye slotted, so
Eagle Eye's weapon-only potency scoping is undisturbed.

**Cause**
Apply ordering in `TgDeviceFire.ApplyHit`: the HP-gated type-505 pass is submitted at lines
1409–1412, before the type-264 pass at 1445 that carries the base damage. Every other 505 dispatch
in the function sits after the damage submit. Fixed server-side by lending the target its
pre-debuff protection back for the duration of that impact's mitigation only.

**Impact**
Was minor — ~4% extra damage on the first shot only, favouring the attacker, and masked by the
health cap against full-HP targets without armour.
