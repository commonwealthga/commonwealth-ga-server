# Extract skill-tree structure (nodes, prereqs, grid layout, icons) for the builder
import sqlite3, json, os, base64
db = sqlite3.connect(r"E:\GA_LOCAL\gaa.db"); db.row_factory = sqlite3.Row
def q(s, a=()): return db.execute(s, a).fetchall()
# The game calls it Cooldown wherever the player sees it; the property table says Recharge
# Time. Renamed at the point names are produced, so it is consistent across the whole console.
PROPRENAME = {4: 'Cooldown', 203: 'Cooldown Modifier'}
def pn(pid):
    if pid in PROPRENAME: return PROPRENAME[pid]
    r = q("SELECT name FROM asm_data_set_properties WHERE prop_id=?", (pid,))
    return (r[0]['name'] if r and r[0]['name'] else str(pid))
CALC = {67: '+', 68: '+%', 69: '-%', 70: '-'}
TREE = {155: 'Balanced', 156: 'Healer', 157: 'Poison', 158: 'Tank', 159: 'Destroyer',
        160: 'Infiltration', 161: 'Marksman', 162: 'Engineer', 163: 'Drones'}
CLASSTREES = {'Assault': [155, 158, 159], 'Medic': [155, 156, 157],
              'Recon': [155, 160, 161], 'Robotics': [155, 162, 163]}
ICONDIR = r"E:\GA_BROADCAST_ICONS\ICONS\SKILLS"

# max rank per skill (points spendable)
ranks = {}
for r in q("SELECT skill_id, COUNT(DISTINCT rank_id) n, MAX(rank) mx FROM asm_data_set_skill_group_ranks GROUP BY skill_id"):
    ranks[r['skill_id']] = max(r['n'] or 1, r['mx'] or 1)

TKIND = {261: 'passive', 264: 'on-hit', 505: 'conditional', 759: 'on-hit', 1104: 'reactive'}
def skillname(sid):
    r = q("SELECT name_msg_translated nm FROM asm_data_set_skill_group_skills WHERE skill_id=? AND name_msg_translated<>'' LIMIT 1", (sid,))
    return r[0]['nm'] if r else ''
# inventory devices grouped by the weapon-skill they belong to, so a gated effect can list
# exactly the devices it reaches (rather than everything the parent skill touches).
devbyskill = {}
for r in q("""SELECT DISTINCT i.device_id d, i.profile_id p, it.skill_id sk, m.message nm
              FROM ga_players_inventory i
              JOIN asm_data_set_items it ON it.item_id=i.device_id
              JOIN asm_data_set_msg_translations m ON m.msg_id=it.name_msg_id
              WHERE i.user_id=2381 AND i.device_id>0 AND it.skill_id>0"""):
    PROFCLS = {680: 'Assault', 567: 'Medic', 681: 'Recon', 679: 'Robotics'}
    devbyskill.setdefault(str(r['sk']), []).append([r['nm'], PROFCLS.get(r['p'], 'Shared')])
for v in devbyskill.values(): v.sort()
def effects_for(grp, sid):
    """Structured effects so the character sheet can aggregate them."""
    out = []
    for s in q("SELECT DISTINCT effect_group_id eg, effect_group_type_value_id t FROM asm_data_set_skill_effect_groups WHERE skill_group_id=? AND skill_id=?", (grp, sid)):
        meta = q("SELECT required_skill_id rsk, situational_type_value_id sit, situational_value sv, lifetime_sec life FROM asm_data_set_effect_groups WHERE effect_group_id=? LIMIT 1", (s['eg'],))
        m = meta[0] if meta else None
        for e in q("SELECT prop_id p, base_value bv, calc_method_value_id c FROM asm_data_set_effects WHERE effect_group_id=?", (s['eg'],)):
            calc = e['c']
            rsk = (m['rsk'] if m else 0) or 0
            val = round(e['bv'], 3)
            # Percentages are stored two ways: whole numbers (10.0 = 10%) and 0-1 fractions
            # (0.4 = 40%, e.g. Power Pool Increase, GroundSpeed slows). Normalise to whole %.
            if calc in (68, 69) and 0 < abs(val) < 1:
                val = round(val * 100, 2)
            out.append({'p': e['p'], 'n': pn(e['p']), 'v': val,
                        'pct': 1 if calc in (68, 69) else 0,
                        'neg': 1 if calc in (69, 70) else 0,
                        'life': round(m['life'], 1) if (m and m['life']) else 0,
                        'kind': TKIND.get(s['t'], 'passive'),
                        # rsk = the device-class this effect is gated to. Two effects on the same
                        # property but different rsk are DIFFERENT stats and must not be summed
                        # (e.g. Shield Strength +40% lifetime on Shields vs Point Tank +30% on Boosts).
                        'rsk': rsk, 'rskn': skillname(rsk) if rsk else ''})
    seen = set(); ded = []
    for e in out:
        k = (e['p'], e['v'], e['pct'], e['neg'], e['kind'], e['rsk'])
        if k in seen: continue
        seen.add(k); ded.append(e)
    return ded

nodes = {}
used_icons = set()
for grp in TREE:
    lst = []
    for r in q("""SELECT skill_id sid, name_msg_translated nm, desc_msg_translated ds,
                  prereq_group_points gp, prereq_skill_id psk, prereq_skill_points psp,
                  group_loc_x x, group_loc_y y, icon_id ic
                  FROM asm_data_set_skill_group_skills
                  WHERE skill_group_id=? AND name_msg_translated<>'' ORDER BY prereq_group_points, group_loc_y, group_loc_x""", (grp,)):
        if r['sid'] in [n['id'] for n in lst]: continue
        lst.append({'id': r['sid'], 'name': r['nm'], 'desc': (r['ds'] or '').strip(),
                    'gp': r['gp'] or 0, 'psk': r['psk'] or 0, 'psp': r['psp'] or 0,
                    'x': r['x'] or 0, 'y': r['y'] or 0, 'icon': r['ic'] or 0,
                    'max': ranks.get(r['sid'], 1), 'fx': effects_for(grp, r['sid'])})
        if r['ic']: used_icons.add(r['ic'])
    nodes[grp] = lst

# embed icons as data URIs (only those used)
icons = {}
missing = []
for ic in sorted(used_icons):
    p = os.path.join(ICONDIR, "%s.png" % ic)
    if os.path.exists(p):
        icons[str(ic)] = base64.b64encode(open(p, 'rb').read()).decode()
    else:
        missing.append(ic)

skilldev = {}
sdpath = r"C:\Users\patri\AppData\Local\Temp\claude\E--GA-LOCAL-Repo\4220e829-c0b4-416e-90e1-0bc04ececb41\scratchpad\skilldev.json"
if os.path.exists(sdpath):
    skilldev = json.load(open(sdpath))
for g, lst in nodes.items():
    for n in lst:
        d = skilldev.get(str(n['id']))
        n['dev'] = d['devices'] if d else []
        n['scope'] = d['scope'] if d else ''

# ---- Base Player Stats -------------------------------------------------------
# Identical for all four classes at level 1 (asm_data_set_bots for the class archetype
# bots 680/567/681/679), plus the un-unequippable HUMAN BASE ATTRIBUTES equip effects
# (device 864 -> asm_data_set_device_effect_groups eg3575).
BOT = q("SELECT hit_points hp, default_power_pool pp, power_pool_regen_per_sec regen, physical_type_value_id phys FROM asm_data_set_bots WHERE bot_id=680 LIMIT 1")[0]
base = [
    {'p': 51,  'n': 'Health',                  'v': BOT['hp'],    'pct': 0, 'src': 'Base'},
    {'p': 243, 'n': 'Power Pool',              'v': BOT['pp'],    'pct': 0, 'src': 'Base'},
    {'p': 244, 'n': 'Power Pool Recharge Rate','v': BOT['regen'], 'pct': 0, 'src': 'Base'},
]
for e in q("""SELECT e.prop_id p, e.base_value bv, e.calc_method_value_id c
              FROM asm_data_set_device_effect_groups deg
              JOIN asm_data_set_effects e ON e.effect_group_id=deg.effect_group_id
              WHERE deg.device_id=864"""):
    base.append({'p': e['p'], 'n': pn(e['p']), 'v': round(e['bv'], 2),
                 'pct': 1 if e['c'] in (68, 69) else 0, 'src': 'Human Base Attributes'})

# ---- Armour --------------------------------------------------------------------
# 7 identical slots (the body-part names are cosmetic). Every piece carries a fixed
# +10% Health Mod plus 6 rolled mods; the letter code is the in-game notation.
ARMLET = {412: 'N', 390: 'N', 218: 'R', 217: 'M', 219: 'B'}
armcfg = {}
for r in q("""SELECT mod_effect_group_ids m FROM ga_players_inventory
              WHERE user_id=2381 AND allowed_slots IN ('1130','1143','1133','1136','1132','1139','1142')
              AND mod_effect_group_ids<>''"""):
    parts = [int(t) for t in r['m'].split(',') if t.strip().isdigit()]
    runs = []
    for eg in parts:
        if runs and runs[-1][0] == eg: runs[-1][1] += 1
        else: runs.append([eg, 1])
    sig = ''; eff = {}
    for i, (eg, cnt) in enumerate(runs):
        fx = q("SELECT prop_id p, base_value bv, calc_method_value_id c FROM asm_data_set_effects WHERE effect_group_id=?", (eg,))
        if not fx: continue
        p, bv, c = fx[0]['p'], fx[0]['bv'], fx[0]['c']
        if not (i == 0 and cnt == 1):
            sig += ARMLET.get(p, '?') * cnt
        k = (p, 1 if c in (68, 69) else 0)
        eff[k] = eff.get(k, 0) + bv * cnt
    if sig and sig not in armcfg:
        armcfg[sig] = [{'p': p, 'n': pn(p), 'v': round(v, 2), 'pct': pct} for (p, pct), v in sorted(eff.items())]
armicon = ''
_ap = r"E:\GA_BROADCAST_ICONS\ICONS\GEAR\Armor.png"
if os.path.exists(_ap):
    armicon = base64.b64encode(open(_ap, 'rb').read()).decode()

out = {'trees': {str(k): v for k, v in nodes.items()}, 'names': {str(k): v for k, v in TREE.items()},
       'classes': CLASSTREES, 'icons': icons, 'maxPoints': 13, 'devbyskill': devbyskill,
       'base': base, 'phys': BOT['phys'],
       'armour': {'configs': armcfg, 'icon': armicon, 'slots': 7, 'default': 'RRRRRR'}}
print("armour configs:", sorted(armcfg.keys()), "icon:", bool(armicon))
print("base stats:", [(b['n'], b['v']) for b in base])
json.dump(out, open(r"C:\Users\patri\AppData\Local\Temp\claude\E--GA-LOCAL-Repo\4220e829-c0b4-416e-90e1-0bc04ececb41\scratchpad\tree.json", 'w'))
print("trees:", {TREE[k]: len(v) for k, v in nodes.items()})
print("icons embedded:", len(icons), "missing:", missing[:10], "(%d)" % len(missing))
print("max ranks seen:", sorted({n['max'] for v in nodes.values() for n in v}))
print("grid extents:", {TREE[k]: (max((n['x'] for n in v), default=0), max((n['y'] for n in v), default=0)) for k, v in nodes.items()})
print("json bytes:", os.path.getsize(r"C:\Users\patri\AppData\Local\Temp\claude\E--GA-LOCAL-Repo\4220e829-c0b4-416e-90e1-0bc04ececb41\scratchpad\tree.json"))
