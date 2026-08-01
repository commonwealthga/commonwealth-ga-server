# Inventory-driven device + modifier model for user 2381 (v2: modes, cure/strip, morale, jetpack filter, HBA base)
import sqlite3, json
db = sqlite3.connect(r"E:\GA_LOCAL\gaa.db"); db.row_factory = sqlite3.Row
def q(s, a=()): return db.execute(s, a).fetchall()
def iname(iid):
    r = q("SELECT m.message FROM asm_data_set_items i JOIN asm_data_set_msg_translations m ON m.msg_id=i.name_msg_id WHERE i.item_id=? LIMIT 1", (iid,))
    return r[0]['message'] if r else ("dev%s" % iid)
def pn(pid):
    r = q("SELECT name FROM asm_data_set_properties WHERE prop_id=?", (pid,))
    return (r[0]['name'] if r and r[0]['name'] else str(pid))
CALC = {67: '+', 68: '+%', 69: '-%', 70: '-'}
PROF = {680: 'Assault', 567: 'Medic', 681: 'Recon', 679: 'Robotics'}
ORDER = ['Assault', 'Medic', 'Recon', 'Robotics']
SLOT = {'221': 'Melee', '198': 'Ranged', '200': 'Specialty', '201': 'Jetpack',
        '203,204,385': 'Offhand', '386': 'Boost', '502': 'Class', '500': 'Consumable'}
# categories that are DEBUFFS/DoT/CC -> removing them = CURE (ally cleanse). Others removed = buff STRIP.
DEBUFF_CATS = {303, 304, 305, 378, 431, 653, 719, 875, 921, 1016}
# prop -> signature letter. Letters are CONTEXT-DEPENDENT (device vs armor) — confirmed against in-game codes.
LET_DEV = {242: 'P',                                    # P = Power (power pool cost)
           65: 'D', 214: 'D', 212: 'D', 321: 'D', 350: 'D', 372: 'D', 373: 'D', 374: 'D', 375: 'D',  # D = damage
           330: 'H', 210: 'H', 51: 'H', 211: 'H',       # H = heal
           386: 'S',                                    # S = shield strength
           208: 'T', 355: 'T',                          # T = duration / lifespan
           339: 'VN', 366: 'VN',                        # VN = deployable / pet health
           352: 'X', 382: 'X',                          # X = AOE radius
           114: 'R', 381: 'R', 207: 'R', 5: 'R', 153: 'R',  # R = range
           357: 'M',                                    # M = morale requirement (boosts)
           10: 'C', 113: 'C', 232: 'F', 376: 'y', 316: 'V', 140: 'K'}
LET_ARM = {412: 'N', 390: 'N',                          # N = health
           218: 'R', 217: 'M', 219: 'B',                # R/M/B = ranged/melee/AOE protection
           155: 'P'}
for r in q("SELECT prop_id FROM asm_data_set_properties WHERE name LIKE '%Recharge Time%'"):
    LET_DEV[r['prop_id']] = 'C'                          # C = cooldown

def decode_mods(csv, armor=False):
    """Return (base_label, groups). Each group = (count, per-mod label, letter, group-total label).
    The game displays the GROUP TOTAL (e.g. 3 mods x -1% shows as -3%)."""
    LET = LET_ARM if armor else LET_DEV
    parts = [t.strip() for t in (csv or '').split(',') if t.strip().isdigit()]
    if not parts: return (None, [])
    runs = []
    for eg in parts:
        eg = int(eg)
        if runs and runs[-1][0] == eg: runs[-1][1] += 1
        else: runs.append([eg, 1])
    base = None; groups = []; nums = []
    for i, (eg, cnt) in enumerate(runs):
        fx = q("SELECT prop_id p, base_value bv, calc_method_value_id calc FROM asm_data_set_effects WHERE effect_group_id=?", (eg,))
        if not fx: continue
        p, bv, calc = fx[0]['p'], fx[0]['bv'], fx[0]['calc']
        pct = '%' if calc in (68, 69) else ''
        sgn = '+' if calc in (67, 68) else '-'
        total = "%s %s%s%s" % (pn(p), sgn, round(bv * cnt, 2), pct)          # what the game shows
        per = "%dx %s%s%s" % (cnt, sgn, round(bv, 2), pct)                    # the maths behind it
        nums.append([p, round(bv * cnt, 3), calc, pn(p)])                     # item-layer total
        if i == 0 and cnt == 1:
            base = "%s %s%s%s" % (pn(p), sgn, round(bv, 2), pct)
        else:
            groups.append((cnt, total, LET.get(p, '?'), per))
    return (base, groups, nums)
def sig(groups): return "".join(l * c for c, tot, l, per in groups) or "—"

HEALP = {51, 211}
PROT = {155: 'Phys', 156: 'Fire', 157: 'Energy', 158: 'Slow', 159: 'Bio', 160: 'Disease', 163: 'Stun',
        168: 'Sleep', 217: 'Melee', 218: 'Ranged', 219: 'AOE', 233: 'KB', 235: 'EMP', 266: 'Ignite', 324: 'Poison', 328: 'EMP-Burn', 371: 'Bleed'}
DMGMOD = {65: 'Dmg', 214: 'Rng Dmg', 212: 'Melee Dmg', 321: 'AOE Dmg', 385: 'Output', 350: 'Pet Dmg', 376: 'Potency', 336: 'All Dmg'}
CCP = {166: 'Stun', 167: 'Slow', 169: 'Sleep', 170: 'Knockback', 171: 'Immobilize', 172: 'Confuse',
       254: 'Tag', 338: 'Root', 305: 'Taunt'}
# Heal modifiers. 210 on a TARGET reduces the healing they receive (Scorpia's anti-heal);
# 330 is healing output. Both were previously unmapped and silently dropped.
HEALMOD = {210: 'Heal Received', 330: 'Healing', 260: 'Repair'}
MISC = {420: 'Threat', 421: 'Threat', 231: 'Melee Atk Rate', 278: 'Deploy Rate',
        50: 'Jump', 59: 'Flight Accel', 136: 'Gravity', 137: 'Falling Dmg'}
SENSORP = {13, 90, 101, 152, 165, 221, 223, 220}

PROTORD = ['Phys', 'Fire', 'Energy', 'Bio', 'Disease', 'Poison', 'Bleed', 'Ignite',
           'Melee', 'Ranged', 'AOE', 'Stun', 'Sleep', 'KB', 'EMP', 'EMP-Burn', 'Slow']
def merge_prots(chips):
    """One effect group often shreds several protection axes by the same amount - the EMP
    Grenade drops Physical, Fire, Energy, Poison and AOE all by 40 for 4s, which the in-game
    tooltip states once as "-40% protection". Collapse identical-value sets into one chip so
    the display matches the tooltip instead of repeating the same number five times."""
    import re
    pat = re.compile(r'^((?:Mech: |Self: )?)([A-Za-z][\w-]*) Prot ([-+][\d.]+%?)(.*)$')
    groups = {}
    order = []
    out = []
    for c in chips:
        m = pat.match(c[1])
        if not m:
            out.append(c); continue
        key = (c[0], m.group(1), m.group(3), m.group(4), tuple(c[2][2:]) if len(c) > 2 else ())
        if key not in groups:
            groups[key] = []
            order.append((key, c))
        groups[key].append(m.group(2))
    for key, proto in order:
        axes = groups[key]
        kind, pfx, val, tail = key[0], key[1], key[2], key[3]
        if len(axes) == 1:
            label = '%s%s Prot %s%s' % (pfx, axes[0], val, tail)
        elif len(axes) >= 5:
            label = '%sAll Prot %s%s' % (pfx, val, tail)
        else:
            axes = sorted(axes, key=lambda a: PROTORD.index(a) if a in PROTORD else 99)
            label = '%s%s Prot %s%s' % (pfx, '/'.join(axes), val, tail)
        out.append([kind, label] + list(proto[2:]))
    return out

def dedup(lst):
    seen = set(); out = []
    for c in lst:
        t = (c[0], c[1])
        if t in seen: continue
        seen.add(t); out.append(list(c))
    return out

RANGEP = {114: 'Range', 207: 'Eff Range', 208: 'Eff Range', 5: 'Range', 153: 'Eff Range'}
ACCP = {10: 'Accuracy', 113: 'Accuracy', 256: 'Accuracy'}
# effect-group-type -> chip context prefix (266 = Aim/scope, 261 = while equipped, 263 = while firing)
EGPFX = {266: 'Zoom: ', 261: '', 263: '', 283: ''}
# ---- category names + gating ------------------------------------------------
def vname(v):
    r = q("SELECT text_msg_translated t FROM asm_data_set_valid_values WHERE value_id=? AND text_msg_translated<>'' LIMIT 1", (v,))
    return r[0]['t'] if r else ''
# CalcCategoryProtection (TgEffectGroup.uc): category -> the protection prop that mitigates it.
CATGATE = {303: 159, 304: 158, 305: 160, 378: 163, 431: 168, 875: 233, 653: 235, 719: 266, 1016: 371, 921: 328}
# Players (HUMAN BASE ATTRIBUTES) carry 1000 in these two => total immunity, so any effect in
# those categories only lands on MECHANICAL targets.
PLAYER_IMMUNE = {653, 921}

def spawned_sources(did):
    """Devices whose effects live on a spawned entity (bomb -> explosion deployable,
    turret/drone -> bot's weapon). Returns [(device_id, label)]."""
    out = []
    for m in q("SELECT device_projectile_id proj, deployable_id dep, bot_id bot FROM asm_data_set_devices_data_set_device_modes WHERE device_id=?", (did,)):
        deps, bots = [], []
        if m['proj']:
            for p in q("SELECT spawn_item_id si, spawn_bot_id sb, spawn_deployable_id sd FROM asm_data_set_projectiles WHERE device_projectile_id=? LIMIT 1", (m['proj'],)):
                if p['sd']: deps.append(p['sd'])
                if p['sb']: bots.append(p['sb'])
                if p['si']: out.append((p['si'], iname(p['si']) or 'Impact'))
        if m['dep']: deps.append(m['dep'])
        if m['bot']: bots.append(m['bot'])
        for d in deps:
            for r in q("SELECT device_id dv, name_msg_translated n FROM asm_data_set_deployables WHERE deployable_id=? LIMIT 1", (d,)):
                if r['dv']: out.append((r['dv'], (r['n'] or 'Deployed').replace('* ', '')))
        for b in bots:
            for r in q("SELECT device_id dv FROM asm_data_set_bots_data_set_bot_devices WHERE bot_id=?", (b,)):
                if r['dv']: out.append((r['dv'], iname(r['dv']) or 'Deployed'))
    seen = set(); ded = []
    for d, lab in out:
        if d in seen or d == did: continue
        seen.add(d); ded.append((d, lab))
    return ded

STATP = {354: ('Lifespan', 's'), 4: ('Cooldown', 's'), 279: ('Deploy', 's'),
         230: ('Max out', ''), 53: ('Refire', 's'), 259: ('Scope', 'x'),
         # 150 Persist Time is how long a deployed thing lives; 5 is its placement range
         150: ('Duration', 's'), 5: ('Range', '')}
def dev_stats(did):
    """Key numbers that live on the device row rather than in an effect group."""
    out = []
    for r in q("SELECT prop_id p, base_value bv FROM asm_data_set_device_mode_properties WHERE device_id=? ORDER BY prop_id", (did,)):
        if r['p'] in STATP:
            lab, unit = STATP[r['p']]
            # 10000s on Persist Time is a sentinel for "until destroyed", not a duration
            if r['p'] == 150 and r['bv'] >= 10000:
                out.append(['stat', 'Lasts until destroyed'])
                continue
            v = round(r['bv'], 2)
            v = int(v) if v == int(v) else v
            out.append(['stat', '%s %s%s' % (lab, v, unit), [r['p'], r['bv'], 67, 0, 0, 0, 0, 0]])
    seen = set(); ded = []
    for c in out:
        if c[1] in seen: continue
        seen.add(c[1]); ded.append(c)
    return ded

def deployable_stats(did):
    """Health of whatever this device deploys - Force Wall 2370, Dome Shield Boost 2500,
    the stations 1500. It lives on asm_data_set_deployables, not in any effect group, so
    without it a pure deployable like Dome Shield Boost has nothing to show at all.
    Carried as prop 339 so the Robotics deployable-health skills scale it."""
    out = []
    for m in q("SELECT DISTINCT deployable_id dep FROM asm_data_set_devices_data_set_device_modes WHERE device_id=? AND deployable_id>0", (did,)):
        for r in q("SELECT health hp FROM asm_data_set_deployables WHERE deployable_id=? LIMIT 1", (m['dep'],)):
            if not r['hp']: continue
            out.append(['hp', 'Structure HP %d' % int(r['hp']), [339, float(r['hp']), 67, 0, 0, 0, 0, 0]])
    seen = set(); ded = []
    for c in out:
        if c[1] in seen: continue
        seen.add(c[1]); ded.append(c)
    return ded

def dev_modes(did, is_melee=False, recurse=True, is_spawn=False):
    out = []
    rcq = q("SELECT right_click_behavior_type_value_id rc FROM asm_data_set_devices WHERE device_id=? LIMIT 1", (did,))
    rc = rcq[0]['rc'] if rcq else 0
    can_block = is_melee and rc == 894          # 894 = RMB blocks; 830 = RMB is an alt-fire
    for m in q("SELECT device_mode_id mid, name_msg_id FROM asm_data_set_devices_data_set_device_modes WHERE device_id=?", (did,)):
        pcr = q("SELECT base_value bv FROM asm_data_set_device_mode_properties WHERE device_id=? AND device_mode_id=? AND prop_id=242", (did, m['mid']))
        power = round(pcr[0]['bv'], 2) if pcr else None
        mn = q("SELECT message msg FROM asm_data_set_msg_translations WHERE msg_id=?", (m['name_msg_id'],))
        mname = mn[0]['msg'] if mn else 'Fire'
        chips = []; zoomchips = []; bschips = []; blkchips = []; sensor = False; remove_pvs = set()
        sensordet = {}
        for eg in q("SELECT DISTINCT effect_group_id eg, effect_group_type_value_id t FROM asm_data_set_device_mode_effect_groups WHERE device_id=? AND device_mode_id=?", (did, m['mid'])):
            egt = eg['t']
            meta = q("SELECT lifetime_sec l, apply_interval_sec iv, situational_type_value_id sit, category_value_id cat, health hp, application_value_id app, application_value appv FROM asm_data_set_effect_groups WHERE effect_group_id=? LIMIT 1", (eg['eg'],))
            life = meta[0]['l'] if meta else 0; iv = (meta[0]['iv'] if meta else 0) or 0
            sitv = meta[0]['sit'] if meta else 0
            egcat = meta[0]['cat'] if meta else 0
            eghp = (meta[0]['hp'] if meta else 0) or 0
            # Stacking rule for this effect's CATEGORY: 155 Stackable, 156 Newest Wins,
            # 157 Strongest Wins, 836 Refresh, 874 Oldest Wins. Anything but Stackable means
            # only one effect group in that category can be live at once -- this is what makes
            # AOE Shield and Range Shield (both cat 770) mutually exclusive.
            egapp = (meta[0]['app'] if meta else 0) or 0
            egappv = (meta[0]['appv'] if meta else 0) or 0
            # An effect group in a player-immune category (EMP Critical Failure / EMP Burn)
            # only lands on MECHANICAL targets — this is how "-15% protection on Mechanical
            # targets" (iMinigun) and the drones' protection shred are implemented.
            mechpfx = 'Mech: ' if egcat in PLAYER_IMMUNE else ''
            # Who receives it? Confirmed from TgDevice.uc constants + call sites:
            #   SELF  (ApplyEffectType(Instigator)): 261 EQUIP, 262 BUILDUP, 263 FIRE,
            #         265 COOLDOWN, 266 AIM, 283 EQUIP_MODE, 759 SUCCESSFUL_HIT, 1104 REACTIVE
            #   TARGET (SubmitHitEffects -> Impact.HitActor): 264 HIT, 272 HIT_IN_AIR, 505 HIT_SITUATIONAL
            #   ATTACKER: 398 BLOCK_HIT, 594 IS_BLOCKED
            # 266 AIM is ApplyEffectType(Instigator, 266) -> lands on YOU while scoped
            selfpfx = 'Self: ' if egt in (261, 262, 263, 265, 266, 283, 759, 1104) else ''
            zoom = (egt == 266)
            # Backstab: type-505 groups matched via SubmitHitEffects(...,505,SituationalMeleeGroupToApply)
            # where GetSituationalMeleeEffectToApply returns 509 (backstab) / 718 (block-breaker).
            backstab = (egt == 505 and sitv == 509 and is_melee)
            # TGEGT_BLOCK_HIT = 398 (TgDevice.uc:15): counter-effect dealt to an ATTACKER who melees
            # you while YOU are blocking with this weapon. NOT part of the normal swing.
            blockhit = (egt == 398)
            sink = zoomchips if zoom else (bschips if backstab else (blkchips if blockhit else chips))
            cur = [0, 0.0, 0, 0, 0]     # numeric backing of the effect currently being formatted
            def add(kind, text, _s=None):
                (_s if _s is not None else sink).append((kind, (selfpfx or mechpfx) + text, list(cur)))
            for e in q("SELECT prop_id p, base_value bv, calc_method_value_id calc, apply_on_interval_flag tick, property_value_id pv FROM asm_data_set_effects WHERE effect_group_id=?", (eg['eg'],)):
                p, bv, calc, tick, pv = e['p'], e['bv'], e['calc'], e['tick'], e['pv']
                pos = calc in (67, 68); pct = calc in (68, 69)
                cur[:] = [p, round(bv, 3), calc, round(life or 0, 2), egt, egcat or 0, egapp, round(egappv, 2)]
                s = '+' if pos else '-'; u = '%' if pct else ''
                dur = " %ss" % round(life, 1) if life and life > 0 else ""
                over = tick or iv > 0
                if p == 0: continue
                if p in SENSORP:
                    # Anti-stealth detection block. calc 119 is "N/A" - a SET, not arithmetic:
                    # you assign a field of view, you do not add to one. Confirmed by the
                    # Sensor Boost tooltip line "Can see enemy stealth for 10 sec".
                    sensor = True
                    if p == 13: sensordet['range'] = bv
                    elif p == 90: sensordet['fov'] = bv
                    elif p == 223: sensordet['los'] = bv
                    elif p == 221: sensordet['cfg'] = bv
                    continue
                if p == 140: remove_pvs.add(pv); continue
                if p in (200, 353, 341, 124): continue
                if p in HEALP:
                    if pos: add('heal', ('HoT ' if over else 'Heal ') + '+%s%s' % (round(bv, 1), dur))
                    elif egt == 263 and is_spawn:
                        # The spawned entity deleting itself (bomb detonating, drone expiring).
                        # "Self" here is the bomb/drone, not the player — nothing to display.
                        continue
                    else:   add('dmg', ('DoT ' if over else 'Dmg ') + '%s%s' % (round(bv, 1), dur))
                elif p in CCP: add('debuff', CCP[p] + dur)
                elif p in HEALMOD:
                    add('heal' if pos else 'debuff', '%s %s%s%s%s' % (HEALMOD[p], s, round(bv, 1), u, dur))
                elif p in MISC:
                    add('util', '%s %s%s%s%s' % (MISC[p], s, round(bv, 1), u, dur))
                elif p in PROT:
                    if pos: add('prot', '%s Prot +%s%s%s' % (PROT[p], round(bv, 1), u, dur))
                    else:   add('debuff', '%s Prot -%s%s%s' % (PROT[p], round(bv, 1), u, dur))
                elif p in DMGMOD: add('dmg' if pos else 'debuff', '%s %s%s%s%s' % (DMGMOD[p], s, round(bv, 1), u, dur))
                elif p == 316: add('debuff', '+Dmg Taken %s%s%s' % (round(bv, 1), u, dur))
                elif p in RANGEP: add('util', '%s %s%s%s' % (RANGEP[p], s, round(bv, 1), u))
                elif p in ACCP: add('util', 'Accuracy %s%s%s' % (s, round(bv, 1), u))
                elif p in (388, 389): add('dmg', 'vs %s %s%s%s' % ('Mech' if p == 388 else 'Bio', s, round(bv, 1), u))
                elif p in (243, 244, 255): add('power', 'Power %s%s%s%s' % (s, round(bv, 1), u, dur))
                elif p in (412, 390): add('hp', 'Max HP %s%s%s%s' % (s, round(bv, 1), u, dur))
                elif p in (49, 66, 70):
                    # prop 49 = GroundSpeed (NOT spread). Fractional %-values are 0-1 (0.5 = 50%).
                    val = round(bv * 100, 0) if (pct and abs(bv) <= 1) else round(bv, 1)
                    if abs(val) < 2 and pct: continue          # trivial 1% chip-slow, noise
                    if p == 70: lbl = 'Air Spd'
                    elif not pos: lbl = 'Slow'
                    else: lbl = 'Speed'
                    add('debuff' if not pos else 'util', '%s %s%s%s%s' % (lbl, s, val, u, dur))
                elif pn(p) in ('Knockback', 'Pushback'): add('debuff', 'Knockback' + dur)
            # The shield POOL lives on the effect group (asm_data_set_effect_groups.health),
            # not in its effects -- AOE Shield health=2000 is the tooltip's "or 2000 damage".
            # Scaled by 386 Effect Shield Modifier ("+12% Shield Health"). Categories 304 Slow
            # (health=1) and 877 Remove Effect use the column for something else.
            if eghp > 1 and egcat not in (304, 877):
                # 386 is the MODIFIER; there is no separate prop id for the pool itself, so the
                # chip carries 386 as its identity.
                sink.append(('prot', (selfpfx or mechpfx) + 'Shield Health %d' % int(eghp),
                             [386, float(eghp), 67, life or 0, egt, egcat or 0, egapp, round(egappv, 2)]))
        if remove_pvs:
            # prop 140 carries the CATEGORY it strips in property_value_id, and devices are
            # specific: Bionics removes Slow only, a Healing Grenade removes Poison/Ignite/
            # Disease, Neutralize Wave strips 15 buff categories. Naming them beats a blanket
            # "cures debuffs", which was wrong for every device that removes just one thing.
            names = [vname(v) for v in sorted(remove_pvs) if vname(v)]
            names = [n for n in names if n]
            cure = bool(remove_pvs & DEBUFF_CATS)
            if not names:
                chips.insert(0, ['heal' if cure else 'debuff', 'Cures debuffs' if cure else 'Strips buffs'])
            elif len(names) <= 3:
                chips.insert(0, ['heal' if cure else 'debuff',
                                 ('Removes ' if cure else 'Strips ') + ' / '.join(names)])
            else:
                chips.insert(0, ['heal' if cure else 'debuff',
                                 ('Removes ' if cure else 'Strips ') + '%d effects' % len(names)])
        if sensor:
            # Sensor Visibility Config (221) says WHAT the sensor reveals - these devices share
            # an ESP layer but are not all stealth detection. Confirmed against the in-game
            # descriptions: 34 "Can see enemies in Stealth", 35 "see enemies through nearby
            # walls", 36 "Enemies below 25% health are highlighted".
            SEECFG = {34: 'See stealth', 35: 'See through walls',
                      36: 'Highlight hurt enemies', 28: 'Sensor / minimap'}
            bits = [SEECFG.get(int(sensordet.get('cfg', 0)), 'Sensor / reveal')]
            if sensordet.get('range'): bits.append('%du' % int(sensordet['range']))
            if sensordet.get('fov'): bits.append('%d°' % int(sensordet['fov']))
            if 'los' in sensordet and int(sensordet.get('cfg', 0)) != 35:
                bits.append('needs LOS' if sensordet['los'] else 'through walls')
            chips.insert(0, ['util', ' '.join(bits)])
        chips = merge_prots(chips)
        chips = dedup(chips)[:8]
        for c in dedup(bschips)[:4]:
            chips.append([c[0], 'Backstab: ' + c[1]] + c[2:])
        out.append({'name': mname, 'power': power, 'chips': chips,
                    'zoom': dedup(zoomchips)[:4], 'blk': dedup(blkchips)[:4], 'mid': m['mid']})

    # ---- Collapse to the game's TWO inputs: PRIMARY (LMB) + ALT (RMB) ----
    # Melee RMB is decided by right_click_behavior_type_value_id:
    #   894 = hold to BLOCK (DeviceBlock state, drains prop 322 power/sec)
    #   830 = a second fire mode instead (Brawler's Heavy Smash, Assassin Blade Heavy Slash)
    # For ranged/specialty with an aim group (type 266), RMB = aim/zoom.
    if not out: return []
    rows = [{'kind': 'PRI', 'name': out[0]['name'], 'power': out[0]['power'],
             'chips': out[0]['chips'] + dev_stats(did) + deployable_stats(did)}]
    if can_block:
        bchips = [['blk', 'Blocks frontal melee']]
        bc = q("SELECT base_value bv FROM asm_data_set_device_mode_properties WHERE device_id=? AND device_mode_id=? AND prop_id=322", (did, out[0]['mid']))
        if bc and bc[0]['bv'] > 0:
            v = bc[0]['bv']
            bchips.append(['power', '%s pwr/sec' % (int(v) if v == int(v) else round(v, 1))])
        for c in out[0]['blk']:
            bchips.append(['blk', 'Counter: ' + c[1]] + c[2:])
        rows.append({'kind': 'BLOCK', 'name': 'Block', 'power': None, 'chips': bchips})
    elif len(out) > 1:
        rows.append({'kind': 'ALT', 'name': out[1]['name'], 'power': out[1]['power'], 'chips': out[1]['chips']})
    elif out[0]['zoom']:
        rows.append({'kind': 'ALT', 'name': 'Aim / Zoom', 'power': None, 'chips': out[0]['zoom']})
    # Effects that live on a spawned entity (explosion deployable, turret/drone weapon).
    if recurse:
        for sd, lab in spawned_sources(did):
            for sub in dev_modes(sd, False, recurse=False, is_spawn=True):
                ch = dedup(sub['chips'])[:9]
                # skip placeholder spawns (e.g. "* Ammo Crate") that carry only stat chips
                if not any(c[0] != 'stat' for c in ch):
                    continue
                mn = sub.get('name') or lab
                # A pet weapon's 'Explode' mode is the despawn routine when its lifespan ends —
                # it harms nobody but the pet, so it isn't player-facing. (The thrown bombs use
                # 'Timed Explosive' for their real blast, so they are unaffected by this.)
                if mn and mn.strip().lower() == 'explode':
                    continue
                nm = ('%s — %s' % (lab, mn)) if mn and mn.lower() not in lab.lower() else lab
                rows.append({'kind': 'SPAWN', 'name': nm, 'power': sub.get('power'), 'chips': ch})
        # a launcher whose effects all live on the spawned entity has an empty PRIMARY row
        rows = [r for r in rows if r['chips'] or r.get('power') is not None] or rows[:1]
    return rows

rows = q("SELECT * FROM ga_players_inventory WHERE user_id=2381")
model = {}
for cls in ORDER:
    prof = [k for k, v in PROF.items() if v == cls][0]
    devs = {}
    for r in rows:
        if r['profile_id'] == prof and r['device_id']:
            devs.setdefault(r['device_id'], []).append(r)
    cats = {}
    for did, variants in devs.items():
        name = iname(did)
        if did == 864: continue  # HBA handled as base
        slotset = sorted(set(v['allowed_slots'] for v in variants))
        cat = SLOT.get(slotset[0], slotset[0])
        if cat == 'Jetpack' and 'Crescent' not in name: continue  # only Crescent used
        oc = any(v['oc'] for v in variants)
        vmods = []
        for v in variants:
            base, groups, nums = decode_mods(v['mod_effect_group_ids'])
            vmods.append({'sig': sig(groups), 'base': base, 'nums': nums,
                          'groups': ["%s  (%s)" % (tot, per) for c, tot, l, per in groups]})
        cats.setdefault(cat, []).append({'id': did, 'name': name, 'cat': cat, 'oc': oc,
                                         'modes': dev_modes(did, is_melee=(cat == 'Melee')), 'variants': vmods})
    model[cls] = cats

# armor (no-profile, item_id keyed)
armor = {}
ARMSLOT = {'1130': 'Head', '1143': 'Shoulder', '1133': 'Chest', '1136': 'Arm', '1132': 'Hand', '1139': 'Leg', '1142': 'Foot'}
for r in rows:
    if not r['profile_id'] and r['allowed_slots'] in ARMSLOT and r['mod_effect_group_ids']:
        base, groups, _nums = decode_mods(r['mod_effect_group_ids'], armor=True)
        armor.setdefault(ARMSLOT[r['allowed_slots']], []).append(
            {'sig': sig(groups), 'groups': ["%s  (%s)" % (tot, per) for c, tot, l, per in groups]})

# HBA base (true equip attributes, from tooltip; Rest mechanic noted separately)
base_attrs = {'equip': [['prot', 'Physical Prot +30'], ['prot', 'EMP-Stun Prot +1000'], ['prot', 'EMP-Burn Prot +1000']],
              'rest': [['heal', 'Rest: +10% missing HP/s'], ['debuff', 'Rest: -100 all Prot'], ['debuff', 'Rest: -50% Damage'], ['debuff', 'Rest: slow + -power']]}

json.dump({'model': model, 'armor': armor, 'base': base_attrs}, open(r"C:\Users\patri\AppData\Local\Temp\claude\E--GA-LOCAL-Repo\4220e829-c0b4-416e-90e1-0bc04ececb41\scratchpad\inv_model.json", 'w'), indent=0)
for cls in ORDER:
    tot = sum(len(v) for v in model[cls].values())
    print(cls, tot, "devices;", {k: len(v) for k, v in model[cls].items()})
print("armor:", {k: len(v) for k, v in armor.items()})
# checks
hb = [x for c in model['Medic'].values() for x in c if 'Healing Grenade' in x['name']]
print("Healing Grenade modes:", hb[0]['modes'] if hb else "?")
bo = [x for c in model['Medic'].values() for x in c if x['name'] == 'Healing Boost']
print("Healing Boost sig:", [v['sig'] for v in bo[0]['variants']] if bo else "?")
