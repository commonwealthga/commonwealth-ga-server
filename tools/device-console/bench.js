// ---------------------------------------------------------------------------
// GA.resolve - device stat resolver.
//
// Standalone and side-effect free: takes a spec, returns numbers. The test bench
// below is one caller; the intended second is the full "build the player"
// simulator, where an attacker spec and a defender spec are both resolved and
// then run through the mitigation stage.
//
// Model (docs/claude/theorycraft-console/damage-pipeline.md):
//   - TWO LAYERS. Rolled mods are the ITEM layer, skills are the SKILL layer, and
//     they MULTIPLY: base x (1 + item%) x (1 + skill%). Verified in-game as
//     1300 x 1.70 armour x 1.25 skills = 2762.
//   - ConvertPropToPropList expands a device property into the modifier set
//     allowed to touch it (damage -> {65, 336, attack-type, 385, 376}).
//   - A modifier carrying a property_value_id is scoped to that effect CATEGORY
//     and may not touch any other. This is the Eagle Eye / Ballista mechanic.
//   - MODIFIER vs EFFECT properties never compose. Killer Instinct's protection
//     debuff is its own second debuff, not a modifier of the weapon's.
//   - A shield's absorb POOL is asm_data_set_effect_groups.health (AOE Shield = 2000, the
//     tooltip's "or 2000 damage"), scaled by 386 Effect Shield Modifier. It is NOT the
//     protection value, and is unrelated to the retired gear-durability mechanic.
// ---------------------------------------------------------------------------
window.GA = window.GA || {};
(function (GA) {
  var CALC_ADD = 67, CALC_PCT_UP = 68, CALC_PCT_DN = 69, CALC_SUB = 70;

  var ALWAYS = { 261: 1, 283: 1 };          // always-on once allocated
  var KINDLAB = { 264: 'on hit', 272: 'on hit (air)', 505: 'conditional', 759: 'on hit',
                  1104: 'reactive', 263: 'on fire', 262: 'while charging',
                  265: 'on cooldown', 266: 'while aiming' };

  var SHIELD_CAT = 770;                     // "Personal Shield" - the value is DURABILITY
  GA.SHIELD_CAT = SHIELD_CAT;
  var PROT = [155,156,157,158,159,160,163,168,217,218,219,233,235,266,324,328,371];
  GA.PROT_PROPS = PROT;
  var CC   = [166,167,169,170,171,172,254,338,305,60,295,316];

  // chip property -> the properties allowed to MODIFY it
  function propList(prop, isNeg, meta, cat) {
    var atk = meta.atk || [];
    var melee = atk.indexOf(170) >= 0 || atk.indexOf(372) >= 0;
    if (prop === 51 || prop === 211) {
      if (!isNeg) return [330, 385, 376];                  // healing output
      var l = [65, 336, 385, 376];                         // damage
      if (meta.pet) l.push(350);
      else if (melee) l.push(212);
      else if (meta.aoe) l.push(321);
      else l.push(214);
      return l;
    }
    if (prop === 386) return [386, 376];                   // shield absorb pool <- Shield Health
    if (prop === 4) return [203];                          // recharge time <- Recharge Time Modifier
    if (prop === 53) return [232, 231];                    // refire <- attack rate (inverse)
    if (prop === 210 || prop === 330 || prop === 260) return [330, 376];
    if (prop === 354) return [355];
    if (prop === 279) return [391, 360, 278];
    if (prop === 339 || prop === 366) return [339, 366];
    if (prop === 114) return meta.pet ? [381, 114] : [114];
    if (prop === 207) return meta.pet ? [381, 207] : [207];
    if (prop === 10 || prop === 113 || prop === 256) return meta.pet ? [383, 113, 256] : [113, 256];
    if (prop === 352 || prop === 382) return meta.pet ? [382, 352] : [352];
    if (prop === 242) return [242];
    if (prop === 322) return [322];
    if (prop === 421 || prop === 420) return [421];
    if (prop === 49) return [66];                          // slow magnitude <- Effect GroundSpeed Modifier
    if (prop === 208) return [208];
    if (prop === 355) return [355];
    if (PROT.indexOf(prop) >= 0 || CC.indexOf(prop) >= 0) return [376];
    return [376];
  }
  // a POSITIVE modifier here makes the device number go DOWN
  var INVERSE = { 53: 1 };
  // properties that APPLY their own effect rather than scaling one
  var EFFECTP = {};
  PROT.concat(CC).concat([51, 211, 412, 390, 243, 244, 255]).forEach(function (p) { EFFECTP[p] = 1; });
  GA.EFFECT_PROPS = EFFECTP;

  // properties where a LOWER number is better for the player
  // Properties where a REDUCTION is the benefit. Includes ones that modify a penalty rather
  // than a benefit: less falling damage, less of a shield's movement penalty, less threat.
  var LOWER = { 4: 1, 53: 1, 279: 1, 242: 1, 322: 1, 203: 1,
                349: 1,   // remote activation time - faster detonation
                357: 1,   // morale required - cheaper boosts
                391: 1,   // pet deploy time
                137: 1,   // falling damage
                421: 1,   // threat
                66: 1,    // effect groundspeed - only ever used to strip a slow
                316: 1 }; // additional damage taken
  // Potency only ever arrives from your own skills, scoped to something you want moved, so a
  // change in either direction is in your favour.
  var ALWAYS_GOOD = { 376: 1 };
  GA.LOWER_BETTER = LOWER;
  GA.ALWAYS_GOOD = ALWAYS_GOOD;
  // ============================ MITIGATION (damage-pipeline S8) ============================
  // Straight from the decompiled UC (TgEffectGroup.uc:745, TgEffectDamage.uc:302). Three axes,
  // each a separate multiply against the firing device's attack rating - they do NOT add.
  //   final = raw
  //         x (1 - categoryProt   / rating)
  //         x (1 - damageTypeProt / rating)
  //         x (1 - attackTypeProt / rating)   // only for normal weapon categories
  // Protection is stored FLAT and is integer-floored before the divide. Protection >= rating
  // is immunity.

  // m_nDamageType -> protection property
  // Who an effect-group lands on, by group type. A skill that fires "because a rifle is out"
  // still applies to whatever you HIT - Killer Instinct's -10 protection is a debuff on the
  // enemy, not a self-inflicted one - so these must never be folded into the carrier's stats.
  //   264 HIT · 272 HIT_IN_AIR · 505 HIT_SITUATIONAL -> the target
  //   398 BLOCK_HIT                                  -> whoever attacked you
  var LANDS_ON_OTHER = { 264: 1, 272: 1, 505: 1, 398: 1 };
  GA.LANDS_ON_OTHER = LANDS_ON_OTHER;

  var DMGTYPE_PROT = { 113: 155, 115: 156, 116: 157, 897: 324 };
  // m_eAttackType -> protection property
  // 3 = AOE. Nothing in the device data declares it - a splash radius (prop 6) is what puts a
  // shot on this axis instead of Ranged, and it REPLACES Ranged rather than stacking. Confirmed
  // by measurement; see the note in gen2.py hit_of().
  var ATKTYPE_PROT = { 1: 217, 2: 218, 3: 219 };
  // m_nCategoryCode -> protection property (CalcCategoryProtection)
  var CAT_PROT = { 303: 159, 304: 158, 305: 160, 378: 163, 431: 168, 875: 233,
                   653: 235, 719: 266, 1016: 371, 921: 328 };
  // the attack-type axis only participates for ordinary weapon damage
  var NORMAL_CATS = { 302: 1, 963: 1 };
  GA.DMGTYPE_PROT = DMGTYPE_PROT; GA.ATKTYPE_PROT = ATKTYPE_PROT; GA.CAT_PROT = CAT_PROT;

  function axis(prot, rating) {
    if (!rating) return 0;                       // no rating -> no reduction path
    var n = Math.floor(prot || 0);
    if (n <= 0) return 0;
    var f = n / rating;
    return f > 1 ? 1 : f;                        // >= rating is immunity, never negative damage
  }

  /* GA.mitigate(raw, hit, prot, opts)
   *   hit  = { cat, damageType, attackType, rating }
   *   prot = prop -> flat protection value (the defender's aggregated sheet)
   *   opts = { extraTaken, maxHP, curHP, isBot, healthCapArmed }
   * Returns the dealt figure plus every axis, so the panel can show WHY. */
  GA.mitigate = function (raw, hit, prot, opts) {
    hit = hit || {}; prot = prot || {}; opts = opts || {};
    // ProtectionModifier bails entirely when the firing device has no usable rating, so a
    // rating under 1 means NO mitigation on any axis - not "divide by something small".
    var rating = hit.rating == null ? 100 : hit.rating;
    if (rating < 1) {
      return { raw: raw, dealt: raw, shown: Math.floor(raw + 0.5), mitigated: 0,
               mitigatedShown: 0, axes: [], capped: false, rating: rating, mitPct: 0 };
    }
    var cat = hit.cat, axes = [];

    // Prop 316 "Additional Damage Taken" multiplies BEFORE mitigation, not after (S10).
    var taken = opts.extraTaken || 0;
    var value = raw * (1 + taken / 100);
    if (taken) axes.push({ name: 'Additional Damage Taken', prop: 316, pct: taken,
                           reduction: -taken / 100, running: value });

    function applyAxis(name, pprop) {
      if (!pprop) return;
      var pv = prot[pprop] || 0;
      var r = axis(pv, rating);
      var before = value;
      value = value * (1 - r);
      // What this axis took off the running figure. CalcProtection hands exactly this to
      // SubmitMitigationDamage, which is what drains a shield's pool - see the server's
      // TgEffectManager__SubmitMitigationDamage. Note it is the WHOLE axis reduction, base
      // protection included, not just the shield's own contribution.
      axes.push({ name: name, prop: pprop, protection: pv, rating: rating,
                  reduction: r, immune: r >= 1, running: value, absorbed: before - value });
    }
    applyAxis('Category', CAT_PROT[cat]);
    applyAxis('Damage type', DMGTYPE_PROT[hit.damageType]);
    if (NORMAL_CATS[cat]) applyAxis('Attack type', ATKTYPE_PROT[hit.attackType]);

    var dealt = value, capped = false;
    // Per-hit health cap (S8): a PLAYER hit while at/near full HP cannot be taken below 10%
    // of max. Bots are exempt, and a target already below the arming threshold has no cap.
    if (opts.healthCapArmed && !opts.isBot && opts.maxHP > 0) {
      var floor = Math.ceil(opts.maxHP * 0.10);
      var cur = (opts.curHP == null ? opts.maxHP : opts.curHP);
      var room = cur - floor;
      if (room <= 0) { dealt = 0; capped = true; }
      else if (dealt > room) { dealt = room; capped = true; }
    }
    // The game rounds the delivered figure half-up to an integer right after mitigation
    // (TgEffectDamage.uc:167  fProratedAmount = float(int(fProratedAmount + 0.5))), which is why
    // the combat log only ever shows whole numbers. Report both so console figures can be
    // compared against a log directly.
    var shown = Math.floor(dealt + 0.5);
    return { raw: raw, dealt: dealt, shown: shown,
             mitigated: raw - dealt, mitigatedShown: Math.floor(raw - dealt + 0.5),
             axes: axes, capped: capped, rating: rating,
             mitPct: raw > 0 ? (raw - dealt) / raw * 100 : 0 };
  };

  // Pull the defender's flat protection values out of an aggregated sheet (builder statsFor).
  // Protection never appears as a percentage - see the note on PROTP in builder.js.
  GA.protectionFrom = function (stats) {
    var out = {};
    Object.keys(stats || {}).forEach(function (k) {
      var st = stats[k];
      if (st.pct) return;                        // flat only
      if (st.rsk) return;                        // gated to a weapon class, not general defence
      out[st.p] = (out[st.p] || 0) + st.total;
    });
    return out;
  };

  GA.lowerIsBetter = function (prop, kind, cat, self) {
    if (LOWER[prop]) return true;
    // a penalty the device inflicts on its own user (shield slow, scope slow)
    if (kind === 'debuff' && (self || cat === 1452)) return true;
    return false;
  };

  function pctOf(v) {
    // percentages are stored two ways: whole (10.0 = 10%) and 0-1 fractions (0.4 = 40%)
    return (Math.abs(v) > 0 && Math.abs(v) < 1) ? v * 100 : v;
  }

  /**
   * resolve(spec) -> { modes, live, extra }
   * spec = { dev, meta, ix, alloc, situational, variant }
   *   variant  a dev.variants[] entry (rolled mods) = the ITEM layer, or null
   */
  GA.resolve = function (spec) {
    var alloc = spec.alloc || {}, meta = spec.meta || {}, ix = spec.ix || [];
    var variant = spec.variant || null;
    var live = ix.filter(function (e) { return alloc[e.sid]; });

    // ---- SKILL layer ----
    function skillMods(prop, isNeg, cat) {
      var want = propList(prop, isNeg, meta, cat);
      var out = [];
      live.forEach(function (e) {
        var always = !!ALWAYS[e.egt];
        if (!always && !spec.situational) return;
        (e.fx || []).forEach(function (f) {
          var fp = f[0], fv = f[1], fc = f[2], fpv = f[3] || 0;
          if (want.indexOf(fp) < 0) return;
          if (fpv && fpv !== cat) return;              // category-scoped modifier
          var pct = (fc === CALC_PCT_UP || fc === CALC_PCT_DN);
          var v = pct ? pctOf(fv) : fv;
          if (fc === CALC_PCT_DN || fc === CALC_SUB) v = -v;
          if (INVERSE[prop]) v = -v;
          out.push({ src: e.skill, tree: e.tree, sid: e.sid, prop: fp, v: v, pct: pct,
                     layer: 'skill', kind: KINDLAB[e.egt] || e.kind.toLowerCase() });
        });
      });
      return out;
    }
    // ---- ACTIVE BUFF layer ----
    // Anything currently buffing the player - a Range Stim's +25% Range Damage, Visual
    // Scanner's +10%, a repair arm's proximity buff. These are class-157 buffs applied at hit
    // time through the same GetBuffedProperty registry as skills, so they SUM with the skill
    // layer rather than forming another multiplicative one.
    function buffMods(prop, isNeg, cat) {
      var want = propList(prop, isNeg, meta, cat);
      var out = [];
      (spec.buffs || []).forEach(function (b) {
        if (want.indexOf(b.p) < 0) return;
        if (b.src === (spec.name || spec.dev.name)) return;   // never buff yourself
        out.push({ src: b.src, prop: b.p, v: b.v, pct: b.pct, layer: 'active', kind: b.kind || 'buff' });
      });
      return out;
    }
    // ---- ITEM layer (rolled mods) ----
    function itemMods(prop, isNeg, cat) {
      if (!variant || !variant.nums) return [];
      var want = propList(prop, isNeg, meta, cat);
      var out = [];
      variant.nums.forEach(function (f) {
        var fp = f[0], fv = f[1], fc = f[2], nm = f[3];
        if (want.indexOf(fp) < 0) return;
        var pct = (fc === CALC_PCT_UP || fc === CALC_PCT_DN);
        var v = pct ? pctOf(fv) : fv;
        if (fc === CALC_PCT_DN || fc === CALC_SUB) v = -v;
        if (INVERSE[prop]) v = -v;
        out.push({ src: nm, prop: fp, v: v, pct: pct, layer: 'item', kind: 'mod' });
      });
      return out;
    }
    // the two layers multiply; flats add on the end
    // THREE multiplicative layers, not two. Output Mod (385) is its OWN layer - it does not
    // sum with the other rolled mods. Measured on a Ballista OC [dddddd] with no skills, against
    // a Human-Base-Attributes-only target:
    //     585 x 1.75 (Output) x 1.21 (Range 12 + Damage 9) = 1238.74 raw
    //     x 0.70 (Physical 30) = 867 dealt  |  x 0.80 (after its own -10 debuff) = 991 dealt
    // Both shots matched to the unit. Summing Output with the rest gives 1146.6 - about 8% low.
    function apply(base, prop, isNeg, cat) {
      var sm = skillMods(prop, isNeg, cat), im = itemMods(prop, isNeg, cat);
      var bm = buffMods(prop, isNeg, cat);
      var sp = 0, ip = 0, op = 0, flat = 0;
      sm.forEach(function (x) { if (x.pct) sp += x.v; else flat += x.v; });
      bm.forEach(function (x) { if (x.pct) sp += x.v; else flat += x.v; });
      im.forEach(function (x) {
        if (!x.pct) { flat += x.v; return; }
        if (x.prop === 385) op += x.v; else ip += x.v;
      });
      var value = Math.abs(base) * (1 + op / 100) * (1 + ip / 100) * (1 + sp / 100) + flat;
      if (value < 0) value = 0;
      return { value: value, mods: im.concat(bm).concat(sm), skillPct: sp, itemPct: ip, outPct: op, flat: flat };
    }

    var modes = (spec.dev.modes || []).map(function (m) {
      var chips = (m.chips || []).map(function (c) {
        var num = c[2];
        if (!num || !num[0]) return { label: c[1], kind: c[0], base: null, mods: [], lifeMods: [] };
        var prop = num[0], base = num[1], calc = num[2], life = num[3], egt = num[4], cat = num[5] || 0;
        // stacking rule for this effect's category (application_value_id / application_value)
        var app = num[6] || 0, appv = num[7] || 0;
        var isNeg = (calc === CALC_PCT_DN || calc === CALC_SUB);
        var isPct = (calc === CALC_PCT_UP || calc === CALC_PCT_DN);
        if (isPct) base = pctOf(base);
        var r = apply(base, prop, isNeg, cat);
        var lr = life > 0 ? apply(life, prop === 354 ? 355 : 208, false, cat) : null;
        var self = /^Self: /.test(c[1]);
        return { label: c[1], kind: c[0], prop: prop, cat: cat, egt: egt, self: self, neg: isNeg,
                 app: app, appv: appv, sign: isNeg ? -1 : 1,
                 base: Math.abs(base), value: r.value, mods: r.mods, isPct: isPct,
                 lower: GA.lowerIsBetter(prop, c[0], cat, self),
                 life: life, lifeVal: lr ? lr.value : life, lifeMods: lr ? lr.mods : [] };
      });
      var pw = null;
      if (m.power !== null && m.power !== undefined) {
        var pr = apply(m.power, 242, false, 0);
        pw = { base: m.power, value: pr.value, mods: pr.mods, lower: true };
      }
      return { kind: m.kind, name: m.name, zoom: !!m.zoom, power: pw, chips: chips };
    });

    // Effects the allocated skills apply IN ADDITION to the device's own.
    var extra = [];
    live.forEach(function (e) {
      var always = !!ALWAYS[e.egt];
      if (!always && !spec.situational) return;
      (e.fx || []).forEach(function (f) {
        if (!EFFECTP[f[0]]) return;
        extra.push({ skill: e.skill, tree: e.tree, prop: f[0], v: f[1], calc: f[2],
                     egt: e.egt,
                     kind: KINDLAB[e.egt] || e.kind.toLowerCase(), detail: e.detail });
      });
    });
    return { modes: modes, live: live, extra: extra };
  };
})(window.GA);

// ---------------------------------------------------------------------------
// GA.playerEffects - what an EQUIPPED, ACTIVE device does to its own user.
//
// Offensive guard: a device's negative effects are aimed at whatever it hits, never at the
// player carrying it. A Poison Aura cannot poison its owner. The only negatives that land on
// the user are ones the game explicitly scopes to self - the `Self:` effect-group types, or a
// self-penalty category such as 1452 "Shield Movement Penalty" (which Super Tank removes).
// ---------------------------------------------------------------------------
(function (GA) {
  var PSTAT = { 412: 'Health Max Modifier', 390: 'Health Mod', 243: 'Power Pool',
                244: 'Power Pool Recharge Rate', 255: 'Power Pool Max',
                49: 'GroundSpeed', 70: 'AirSpeed', 66: 'GroundSpeed',
                // Offensive/utility modifiers a device grants to YOU. These are real player
                // stats (Sensor Boost = +40% Melee/Range/AoE damage and +20% speed) and were
                // previously dropped because the whitelist only covered defensive stats.
                212: 'Damage Modifier - Melee', 214: 'Damage Modifier - Range',
                321: 'Damage Modifier - AoE', 350: 'Pet Damage Modifier',
                65: 'Effect Damage Modifier', 336: 'All Damage Modifier',
                330: 'Effect Healing Modifier', 376: 'Effect Potency Modifier',
                232: 'Attack Rate Modifier - Ranged', 231: 'Attack Rate Modifier - Melee',
                203: 'Recharge Time Modifier', 242: 'Power Pool Cost',
                208: 'Effect Lifetime Modifier', 355: 'Pet LifeSpan Modifier',
                386: 'Effect Shield Modifier', 114: 'Device Range Modifier',
                207: 'Device Effective Range Modifier', 113: 'Accuracy Modifier',
                256: 'Accuracy Correction Rate Modifier', 366: 'Pet Max Health Modifier',
                339: 'Health Max Deployables', 352: 'AOE Radius Modifier',
                357: 'Required Morale Points Modifier', 421: 'Threat Modifier' };
  var PROTNAME = { 155: 'Protection - Physical', 156: 'Protection - Fire', 157: 'Protection - Energy',
    158: 'Protection - Slow', 159: 'Protection - Biological', 160: 'Protection - Disease',
    163: 'Protection - Stun', 168: 'Protection - Sleep', 217: 'Protection - Melee',
    218: 'Protection - Ranged', 219: 'Protection - AOE', 233: 'Protection - Knockback',
    235: 'Protection - EMP', 266: 'Protection - Ignite', 324: 'Protection - Poison',
    328: 'Protection - EMP-Burn', 371: 'Protection - Bleed' };
  // categories whose effects the device inflicts on its OWN user
  var SELF_PENALTY = { 1452: 1 };
  GA.STACK_RULES = { 155: 'Stackable', 156: 'Newest Wins', 157: 'Strongest Wins',
                     836: 'Refresh', 874: 'Oldest Wins' };
  GA.statName = function (p) { return PROTNAME[p] || PSTAT[p] || null; };

  // ---- repair arms -> turrets / drones -------------------------------------
  // A.R.C. and Focused Repair Arms do three things to a deployable, each on its own effect
  // category: repair it (772 Regeneration, prop 260), buff its damage (935 Proximity Damage
  // Buff, prop 65) and accelerate its deployment (1025 Deploy Time Modifier, prop 278).
  // Category 935 is Strongest Wins, so two arms do not stack - the better buff applies.
  // Deploy time is WORK, not seconds: prop 279 divided by (1 + DeployRate).
  GA.PET_SUPPORT = { dmg: 65, repair: 260, rate: 278 };
  GA.supportFromArms = function (arms) {
    var best = { dmg: 0, repair: 0, rate: 0, src: {} };
    (arms || []).forEach(function (a) {
      (a.chips || []).forEach(function (c) {
        if (c.prop === 65 && c.value > best.dmg) { best.dmg = c.value; best.src.dmg = a.name; }
        if (c.prop === 260 && c.value > best.repair) { best.repair = c.value; best.src.repair = a.name; }
        if (c.prop === 278 && c.value > best.rate) { best.rate = c.value; best.src.rate = a.name; }
      });
    });
    return best;
  };
  // flatten a resolved device to its numeric chips, for the arm scan above
  GA.flatChips = function (res) {
    var out = [];
    res.modes.forEach(function (m) { m.chips.forEach(function (c) { if (c.base !== null) out.push(c); }); });
    return out;
  };

  /* What a device throws AT whoever it is used on, rather than at its carrier.
   * The mirror image of playerEffects: same resolve, same mode filter, but it keeps exactly
   * the groups that land on someone else and drops the ones that stay home.
   *
   * Damage itself (51/211) is deliberately excluded - that is the shot, resolved separately
   * through the mitigation stage. What comes out here is the debuffs and buffs that ride
   * alongside it: a rifle's -10 protection, a wave's heal, a stim's damage boost. */
  // Who a device may be pointed at, straight off the device mode. 'self' devices project
  // nothing at anyone else; 'friend' devices can buff a team-mate; 'enemy' devices debuff the
  // other side. See the TGT_MAP note in gen2.py for why the effect-group type is not enough.
  GA.deviceTarget = function (dev, mode) {
    var ms = (dev && dev.modes) || [];
    for (var i = 0; i < ms.length; i++) {
      if (!mode || !ms[i].kind || ms[i].kind === mode) {
        if (ms[i].hit && ms[i].hit.tgt) return ms[i].hit.tgt;
      }
    }
    return (ms[0] && ms[0].hit && ms[0].hit.tgt) || 'enemy';
  };

  GA.projectedEffects = function (spec) {
    var res = GA.resolve({ dev: spec.dev, meta: spec.meta, ix: spec.ix, alloc: spec.alloc,
                           situational: true, variant: spec.variant, buffs: spec.buffs });
    var want = spec.mode || null;
    if (want) {
      res = { modes: res.modes.filter(function (m) {
                return !m.kind || m.kind === want || m.kind === 'SPAWN';
              }), live: res.live, extra: res.extra };
    }
    var src = spec.name || spec.dev.name, out = [];
    var tgt = GA.deviceTarget(spec.dev, want);
    if (tgt === 'self') return out;                       // nothing leaves the carrier
    res.modes.forEach(function (m) {
      m.chips.forEach(function (c) {
        if (c.base === null || c.prop === 386) return;
        if (c.prop === 51 || c.prop === 211) return;      // that is the shot, not a rider
        if (/^Equip: /.test(c.label)) return;
        if (c.self || SELF_PENALTY[c.cat]) return;        // stays on the carrier
        if (!(GA.LANDS_ON_OTHER || {})[c.egt]) return;    // only groups aimed at someone else
        var nm = GA.statName(c.prop);
        if (!nm) return;
        out.push({ p: c.prop, name: nm, v: c.neg ? -c.value : c.value, pct: c.isPct,
                   src: src, kind: c.neg ? 'debuff' : 'buff', life: c.lifeVal || c.life,
                   cat: c.cat, tgt: tgt });
      });
    });
    res.extra.forEach(function (x) {
      if (!(GA.LANDS_ON_OTHER || {})[x.egt]) return;
      var nm = GA.statName(x.prop);
      if (!nm) return;
      var pos = (x.calc === 67 || x.calc === 68);
      out.push({ p: x.prop, name: nm, v: pos ? x.v : -x.v, pct: (x.calc === 68 || x.calc === 69),
                 src: x.skill, via: src, kind: pos ? 'buff' : 'debuff', life: 0, cat: 0,
                 tgt: tgt });
    });
    return out;
  };

  GA.playerEffects = function (spec) {
    var res = GA.resolve({ dev: spec.dev, meta: spec.meta, ix: spec.ix, alloc: spec.alloc,
                           situational: true, variant: spec.variant, buffs: spec.buffs });
    // A weapon fires primary OR alt, never both, so only the chosen mode contributes.
    // Modes with no kind (and SPAWN/BLOCK follow-ons of the chosen one) are always included.
    var want = spec.mode || null;
    if (want) {
      var scopedP = want === 'ALT' && res.modes.some(function (m) { return m.kind === 'ALT' && m.zoom; });
      res = { modes: res.modes.filter(function (m) {
                return !m.kind || m.kind === want || m.kind === 'SPAWN'
                  || (scopedP && m.kind === 'PRI');
              }), live: res.live, extra: res.extra };
    }
    var src = spec.name || spec.dev.name;
    var effects = [], shields = [];
    // skills that fire because this device is up (Aegis Armament while a shield is active)
    res.extra.forEach(function (x) {
      // declared in the resolver IIFE above, reached here through the namespace
      if ((GA.LANDS_ON_OTHER || {})[x.egt]) return;   // this one hits the enemy, not its carrier
      // An always-on skill passive (Super Agent's +4 protection, egt 261 EQUIP) is already in the
      // skill-tree total. Adding it again here because a device happens to be switched on counted
      // it twice - every Medic device appeared to grant +4 Physical.
      if (x.kind === 'passive') return;
      var nm = GA.statName(x.prop);
      if (!nm) return;
      var pos = (x.calc === 67 || x.calc === 68);
      effects.push({ p: x.prop, name: nm, v: pos ? x.v : -x.v, pct: (x.calc === 68 || x.calc === 69),
                     src: x.skill, via: src, kind: x.kind, life: 0, cat: 0, app: 155, appv: 0 });
    });
    res.modes.forEach(function (m) {
      m.chips.forEach(function (c) {
        if (c.base === null || c.prop === 386) return;
        // "Equip:" passives are collected separately, from every carried device rather than
        // only the active ones. Counting them here too would double them the moment the
        // device is switched on.
        if (/^Equip: /.test(c.label)) return;
        var selfScoped = c.self || SELF_PENALTY[c.cat];
        if (c.neg && !selfScoped) return;            // offensive - never lands on the owner
        var nm = GA.statName(c.prop);
        if (!nm) return;
        effects.push({ p: c.prop, name: nm, v: c.neg ? -c.value : c.value, pct: c.isPct, src: src,
                       kind: c.neg ? 'self penalty' : 'device', life: c.lifeVal || c.life,
                       cat: c.cat, app: c.app, appv: c.appv });
      });
      m.chips.filter(function (c) { return c.prop === 386 && c.base !== null; }).forEach(function (pool) {
        var types = m.chips.filter(function (c) {
          return c.prop !== 386 && c.cat === pool.cat && !c.neg && GA.PROT_PROPS.indexOf(c.prop) >= 0;
        }).map(function (c) { return c.prop; });
        shields.push({ pool: pool.value, life: pool.lifeVal || pool.life, types: types, src: src,
                       cat: pool.cat, app: pool.app, appv: pool.appv });
      });
    });
    return { effects: effects, shields: shields };
  };

  /**
   * Apply the game's stacking rules across everything currently active.
   * Scope is the effect CATEGORY; `application_value_id` decides the winner. Anything but
   * Stackable means only one source in that category survives - this is why AOE Shield and
   * Range Shield (both category 770, Newest Wins) cannot both be up.
   */
  // Category 302 is named "<Local>" in the data - it is the generic marker a device puts on its
  // own direct heal/damage component, not a mutual-exclusion group. Nearly every device carries
  // one, so treating it as a real bucket made unrelated devices fight: Healing Boost's 302 group
  // is "Strongest Wins", which was evicting Protection Wave and Power Wave outright even though
  // their real effects live in different categories (771 Proximity Shield, 1283 Power Pool Buff).
  // human names for effect categories, so a stacking note can say WHICH bucket clashed
  GA.CAT_NAMES = {"1016": "Bleed", "1025": "Deploy Time Modifier", "1095": "Rest", "1283": "Power Pool Buff", "1324": "Station Healing", "1325": "Station Power", "1326": "Station Damage", "1327": "Station Protection", "1341": "Self Heal", "1360": "Movement Penalty", "1452": "Shield Movement Penalty", "1573": "Consumable Buff", "1589": "Regen Damage Penalty", "1601": "Threat Modifier", "302": "<Local>", "303": "Poison", "304": "Slow", "305": "Disease", "378": "Stun", "431": "Sleep", "607": "Ability", "621": "Stealth", "653": "EMP Critical Failure", "719": "Ignite", "768": "EMP Short Circuit", "769": "General Debuff", "770": "Personal Shield", "771": "Proximity Shield", "772": "Regeneration", "773": "Stim Boost", "774": "Stim Resistance", "775": "General Buff", "845": "Sensor", "862": "Invulnerable", "875": "Knockback", "877": "Remove Effect", "879": "Power Pool Debuff", "886": "Morale Buff", "921": "EMP Burn", "935": "Proximity Damage Buff ", "936": "Personal Damage Buff", "985": "Damage Reflect", "986": "Additional Damage"};
  var NOT_A_BUCKET = { 302: 1 };
  GA.applyStacking = function (items, order) {
    var byCat = {};
    items.forEach(function (it) {
      if (!it.cat || it.app === 155 || !it.app) return;   // stackable or uncategorised
      if (NOT_A_BUCKET[it.cat]) return;                   // generic marker, never contends
      (byCat[it.cat] = byCat[it.cat] || []).push(it);
    });
    var suppressed = [], notes = [];
    Object.keys(byCat).forEach(function (cat) {
      var group = byCat[cat];
      var srcs = {};
      group.forEach(function (it) { srcs[it.src] = 1; });
      var names = Object.keys(srcs);
      if (names.length < 2) return;                       // one source, nothing to resolve
      var rule = group[0].app, win;
      if (rule === 157) {                                  // Strongest Wins
        win = names[0]; var best = -Infinity;
        names.forEach(function (n) {
          var v = 0;
          group.forEach(function (it) { if (it.src === n) v = Math.max(v, Math.abs(it.appv || it.v || 0)); });
          if (v > best) { best = v; win = n; }
        });
      } else if (rule === 874) {                           // Oldest Wins
        win = names.slice().sort(function (a, b) { return order.indexOf(a) - order.indexOf(b); })[0];
      } else {                                             // Newest Wins / Refresh
        win = names.slice().sort(function (a, b) { return order.indexOf(b) - order.indexOf(a); })[0];
      }
      names.forEach(function (n) { if (n !== win) suppressed.push(cat + '|' + n); });
      notes.push({ cat: cat, rule: GA.STACK_RULES[rule] || String(rule), win: win,
                   lost: names.filter(function (n) { return n !== win; }) });
    });
    return { blocked: suppressed, notes: notes };
  };
})(window.GA);

// ---------------------------------------------------------------------------
// GA.deviceChipsHTML - the resolved stat chips for one device, as the test bench
// used to draw them: base -> modified, duration, and a tooltip naming every
// contributor and the layer it came from. Rendered inline on each equipped slot.
// ---------------------------------------------------------------------------
(function (GA) {
  var NUMTAIL = new RegExp('\\s[-+]?[\\d.]+(%|x|s)?$');
  var SECS = { 4: 1, 53: 1, 279: 1, 354: 1, 208: 1, 150: 1 };
  var MTAG = { PRI: ['pri', 'PRIMARY'], ALT: ['alt', 'ALT'], BLOCK: ['blkt', 'BLOCK'],
               SPAWN: ['spawn', 'ON IMPACT'], ZOOM: ['alt', 'SCOPED'] };
  function esc(x) {
    return String(x).replace(/[&<>"]/g, function (c) {
      return ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[c];
    });
  }
  function n(v) { return String(Math.round(v * 100) / 100); }
  function sgn(v) { return (v > 0 ? '+' : '') + n(v); }
  function unitFor(c) {
    if (c.isPct) return '%';
    if (SECS[c.prop]) return 's';
    if (c.prop === 259) return 'x';
    return '';
  }
  function label(c) {
    var t = c.label;
    if (c.life > 0) t = t.replace(NUMTAIL, '');
    t = t.replace(NUMTAIL, '');
    return t;
  }
  function delta(lower, val, base, unit, sg) {
    sg = sg || '';
    if (Math.abs(val - base) <= 0.005) return '<b>' + sg + n(base) + (unit || '') + '</b>';
    var good = lower ? (val < base) : (val > base);
    var gone = lower && val === 0;
    return '<b class="was">' + sg + n(base) + (unit || '') + '</b><i class="arw">&rarr;</i>'
      + '<b class="' + (good ? 'up' : 'dn') + '">' + (gone ? 'removed' : sg + n(val) + (unit || '')) + '</b>';
  }

  // Device-level stats (cooldown, deploy, lifespan) belong to the DEVICE, not to any one
  // fire mode, so they are lifted into a header row. A duration shared by every timed effect
  // is lifted too - Regeneration's HoT and its damage penalty both run for the device's 15s,
  // and printing "for 15s" on each of them just repeats the same fact.
  var DEVSTAT = { 4: 'Cooldown', 279: 'Deploy', 354: 'Lifespan', 259: 'Scope', 53: 'Refire',
                  150: 'Duration', 5: 'Range' };

  GA.deviceChipsHTML = function (spec) {
    var res = GA.resolve({ dev: spec.dev, meta: spec.meta, ix: spec.ix, alloc: spec.alloc,
                           situational: !!spec.situational, variant: spec.variant,
                           buffs: spec.buffs });
    var want = spec.mode || null;
    // A scope ALT is a modifier state, not a second attack: a scoped Ballista still fires the
    // same shot, so the primary stays visible with the scope's effects added to it.
    var scoped = want === 'ALT' && res.modes.some(function (m) { return m.kind === 'ALT' && m.zoom; });
    var modes = res.modes.filter(function (m) {
      return !want || !m.kind || m.kind === want || m.kind === 'SPAWN'
        || (scoped && m.kind === 'PRI');
    });

    // pull the device-level stats out of the mode rows
    var head = [], lifeSet = {}, lifeN = 0;
    modes.forEach(function (m) {
      m.chips.forEach(function (c) {
        if (c.base === null) return;
        // a zero device stat is noise (spawned turret weapons carry Cooldown 0)
        if (DEVSTAT[c.prop]) { if (c.base || c.value) head.push(c); return; }
        if (c.life > 0) { var k = Math.round(c.lifeVal * 100); if (!lifeSet[k]) { lifeSet[k] = c; lifeN++; } }
      });
    });
    var oneLife = (lifeN === 1) ? lifeSet[Object.keys(lifeSet)[0]] : null;
    var sup = spec.support && (spec.support.dmg || spec.support.rate || spec.support.repair)
      ? spec.support : null;

    function chipHTML(c, hideLife) {
      // An active repair arm buffs the deployable it is pointed at. Damage is a percentage
      // (category 935); deploy TIME is work divided by (1 + DeployRate), which is why a turret
      // that reads 25 comes up in a few seconds once you weld it.
      if (sup) {
        if (sup.dmg && (c.prop === 51 || c.prop === 211) && c.neg) {
          c = Object.assign({}, c, { value: c.value * (1 + sup.dmg / 100),
              mods: c.mods.concat([{ src: sup.src.dmg, v: sup.dmg, pct: true, layer: 'repair arm', kind: 'support' }]) });
        } else if (sup.rate && c.prop === 279) {
          c = Object.assign({}, c, { value: c.value / (1 + sup.rate),
              mods: c.mods.concat([{ src: sup.src.rate, v: sup.rate, pct: false, layer: 'repair arm', kind: 'deploy rate' }]) });
        }
      }
      var vCh = Math.abs(c.value - c.base) > 0.005;
      var lCh = c.life > 0 && Math.abs(c.lifeVal - c.life) > 0.005;
      var removed = c.lower && c.value === 0 && vCh;
      // On health (51/211) a negative calc MEANS damage - the label already reads "Dmg"/"DoT",
      // so a minus there would render as negative damage. Signs belong on reductions.
      var sg = (c.sign < 0 && c.prop !== 51 && c.prop !== 211) ? '-' : '';
      var body = '<span class="bnlab">' + esc(label(c)) + '</span>'
        + delta(c.lower, c.value, c.base, unitFor(c), sg);
      if (c.life > 0 && !removed && !hideLife) {
        body += '<span class="bnlife"><i class="bnk">for</i>' + delta(c.lower, c.lifeVal, c.life, 's', '') + '</span>';
      }
      var all = c.mods.concat((c.lifeMods || []).map(function (x) {
        return { src: x.src, v: x.v, pct: x.pct, layer: x.layer, kind: 'duration' };
      }));
      var tip = label(c) + ' - base ' + sg + n(c.base) + unitFor(c)
        + (c.prop === 279
            ? '\nBuild time, divided by (1 + DeployRate) while a repair arm welds it.'
              + '\nbombs 0.001 | drones 1 | mines 4 | stations 15'
              + '\nPersonal & Flame turret 25 | Auto Cannon & Rocket turret 40'
              + '\nArms carry DeployRate +4.5 (Nanite Repair +3.5), so 25 becomes 4.5s.'
            : '')
        + (all.length ? '\n\n' + all.map(function (x) {
            return '  [' + x.layer + '] ' + x.src + '  ' + sgn(x.v) + (x.pct ? '%' : '') + '  (' + x.kind + ')';
          }).join('\n') : '\n\nnothing equipped or allocated modifies this');
      return '<span class="bnchip ' + esc(c.kind) + ((vCh || lCh) ? ' hot' : '')
        + '" title="' + esc(tip) + '">' + body + '</span>';
    }

    var headHTML = '';
    var hbits = head.map(function (c) { return chipHTML(c, true); });
    if (oneLife) {
      hbits.push('<span class="bnchip stat' + (Math.abs(oneLife.lifeVal - oneLife.life) > 0.005 ? ' hot' : '')
        + '" title="Every timed effect on this device runs for the same duration"><span class="bnlab">Duration</span>'
        + delta(false, oneLife.lifeVal, oneLife.life, 's', '') + '</span>');
    }
    if (sup) {
      var bits = [];
      // only advertise channels this device can use - a Medical Station gains repair and
      // deploy speed from an arm, it has no damage to buff
      // Ask the resolved chips, not devmeta: a turret is a BOT, so its damage lives on the
      // spawned weapon and devmeta.dmg is false for the device itself.
      var dealsDmg = modes.some(function (m) {
        return m.chips.some(function (c) { return c.base !== null && c.neg && (c.prop === 51 || c.prop === 211); });
      });
      if (sup.dmg && dealsDmg) bits.push('+' + Math.round(sup.dmg) + '% damage');
      // a DoT's damage is snapshot when it lands (TgEffectDamage m_fBuffedDamageInitial), so the
      // buff only raises effects applied while the arm is welding - never a burn already ticking
      if (sup.repair) bits.push(Math.round(sup.repair) + ' repair/tick');
      if (sup.rate) bits.push('deploy /' + (1 + sup.rate).toFixed(1));
      hbits.push('<span class="bnchip sup" title="An active repair arm is supporting this deployable.'
        + (sup.dmg && dealsDmg ? ' The damage buff applies to effects this device applies WHILE the arm is'
            + ' welding - a damage-over-time already ticking on a target keeps the value it was'
            + ' given when it landed.' : '') + '">'
        + '<span class="bnlab">' + esc(sup.src.dmg || sup.src.rate || 'repair arm') + '</span>'
        + esc(bits.join(' · ')) + '</span>');
    }
    if (hbits.length) headHTML = '<div class="bnhead2">' + hbits.join('') + '</div>';

    var body = modes.map(function (m) {
      var tag = (m.zoom ? MTAG.ZOOM : MTAG[m.kind]) || MTAG.PRI;
      var cells = m.chips.map(function (c) {
        if (c.base === null) {
          // gen2 emits the block drain as a plain label with no numeric payload. It is a
          // per-SECOND cost, so pool / rate is how long the block can be held.
          var pw2 = /^([\d.]+) pwr\/sec$/.exec(c.label);
          if (pw2 && window.__PWPOOL__) {
            var rate = parseFloat(pw2[1]);
            var secs = window.__PWPOOL__ / rate;
            var tip2 = 'Power pool ' + window.__PWPOOL__ + ' / ' + rate + ' per second.' + '\n\n'
              + 'Passive regeneration does not run while power is being consumed (confirmed in '
              + 'game), so this is the full sustain. An active source - Power Wave, a Power Stim '
              + 'injection - still tops you up and would extend it.';
            return '<span class="bnchip power" title="' + esc(tip2) + '">'
              + '<span class="bnlab">' + esc(c.label) + '</span><b class="dn">'
              + (Math.round(secs * 10) / 10) + 's</b><i class="bnk">to empty</i></span>';
          }
          return '<span class="bnchip flat">' + esc(c.label) + '</span>';
        }
        if (DEVSTAT[c.prop]) return '';
        if (c.prop === 322 && c.value > 0 && window.__PWPOOL__) {
          // a per-second block cost, so pool / cost is how long the block holds
          var bs = window.__PWPOOL__ / c.value;
          return '<span class="bnchip power' + (Math.abs(c.value - c.base) > 0.005 ? ' hot' : '')
            + '" title="' + esc('Blocking costs ' + n(c.value) + ' power per second'
                + (Math.abs(c.value - c.base) > 0.005 ? ' (base ' + n(c.base) + ', reduced by your melee skills)' : '')
                + '. A ' + window.__PWPOOL__ + ' pool holds it for ' + n(bs) + 's.')
            + '"><span class="bnlab">block</span>'
            + delta(true, c.value, c.base, '/s', '') + '<span class="bnlife"><i class="bnk">holds</i><b class="dn">'
            + n(bs) + 's</b></span></span>';
        }
        return chipHTML(c, !!oneLife);
      }).join('');
      var pw = '';
      if (m.power) {
        pw = '<span class="bnchip power' + (Math.abs(m.power.value - m.power.base) > 0.005 ? ' hot' : '')
          + '"><span class="bnlab">power</span>' + delta(true, m.power.value, m.power.base, '', '') + '</span>';
        // Firing continuously costs power_cost / refire per second. Passive regeneration does
        // not run while power is being spent (confirmed in game), so pool / drain is how long
        // you can hold the trigger down.
        var rf = null;
        m.chips.forEach(function (c) { if (c.prop === 53 && c.base !== null) rf = c.value; });
        if (rf && rf > 0 && m.power.value > 0) {
          var rate = m.power.value / rf;
          var pool = window.__PWPOOL__ || 0;
          var secs = pool ? pool / rate : 0;
          GA.lastDrain = Math.max(GA.lastDrain || 0, rate);
          pw += '<span class="bnchip power" title="' + esc(n(m.power.value) + ' power per shot / '
              + n(rf) + 's refire = ' + n(rate) + ' per second.'
              + (pool ? '\n\nA ' + pool + ' pool sustains ' + n(secs) + 's of continuous fire. '
                      + 'Passive regeneration does not run while power is being spent, so an '
                      + 'active source (Power Wave, a Power Stim injection) is what extends it.' : ''))
            + '"><span class="bnlab">drain</span><b class="dn">' + n(rate) + '/s</b>'
            + (pool ? '<span class="bnlife"><i class="bnk">empties in</i><b class="dn">'
                      + n(secs) + 's</b></span>' : '') + '</span>';
        }
      }
      if (!cells && !pw) return '';
      return '<div class="bnmode"><span class="mtag ' + tag[0] + '">' + tag[1] + '</span>'
        + '<div class="bnchips">' + cells + pw + '</div></div>';
    }).join('');
    return headHTML + body;
  };
})(window.GA);
