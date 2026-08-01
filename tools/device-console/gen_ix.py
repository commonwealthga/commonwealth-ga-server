# Systematic skill -> device interaction resolver.
# Methodology (proven from code):
#  1. required_skill_id gating: buff entries scoped to a skill only match devices whose item skill_id == that skill
#     (GetBuffIndex/GetBuffedProperty match rule: stored skillId>0 only matches same-skill queries).
#  2. required_category gating: reactive skills (ProcessReactiveSkillBasedEffectGroup) trigger while an effect
#     group of that category is active -> devices that produce that category.
#  3. effect_group_type: 261 passive-equip, 264 on-hit, 505 hit-situational (conditional), 759 successful-hit, 1104 reactive.
#  4. Unscoped passives resolve by prop semantics (214 ranged dmg -> ranged devices, 350 pet -> pet devices, etc).
import sqlite3, json
OUT = r'C:/Users/patri/AppData/Local/Temp/claude/E--GA-LOCAL-Repo/4220e829-c0b4-416e-90e1-0bc04ececb41/scratchpad/'
db = sqlite3.connect(r"E:\GA_LOCAL\gaa.db"); db.row_factory = sqlite3.Row
def q(s, a=()): return db.execute(s, a).fetchall()
# The game calls it Cooldown wherever the player sees it; the property table says Recharge
# Time. Renamed at the point names are produced, so it is consistent across the whole console.
PROPRENAME = {4: 'Cooldown', 203: 'Cooldown Modifier'}
def pn(pid):
    if pid in PROPRENAME: return PROPRENAME[pid]
    r = q("SELECT name FROM asm_data_set_properties WHERE prop_id=?", (pid,))
    return (r[0]['name'] if r and r[0]['name'] else str(pid))
def sname(sid):
    r = q("SELECT name_msg_translated nm FROM asm_data_set_skill_group_skills WHERE skill_id=? AND name_msg_translated<>'' LIMIT 1", (sid,))
    return r[0]['nm'] if r else ("skill%s" % sid)
CALC = {67: '+', 68: '+%', 69: '-%', 70: '-'}
TREE = {155: 'Balanced', 156: 'Healer', 157: 'Poison', 158: 'Tank', 159: 'Destroyer',
        160: 'Infiltration', 161: 'Marksman', 162: 'Engineer', 163: 'Drones'}
TREECLS = {155: 'All', 156: 'Medic', 157: 'Medic', 158: 'Assault', 159: 'Assault',
           160: 'Recon', 161: 'Recon', 162: 'Robotics', 163: 'Robotics'}
PROF = {680: 'Assault', 567: 'Medic', 681: 'Recon', 679: 'Robotics'}

# ---------- 1. inventory devices + classification ----------
inv = q("SELECT DISTINCT device_id, profile_id FROM ga_players_inventory WHERE user_id=2381 AND device_id>0")
radius_props = [r['prop_id'] for r in q("SELECT prop_id FROM asm_data_set_properties WHERE name LIKE '%Effect Radius%' OR name='Radius'")]
devices = {}
for r in inv:
    did = r['device_id']
    if did in devices:
        continue
    it = q("SELECT i.skill_id sk, m.message nm FROM asm_data_set_items i JOIN asm_data_set_msg_translations m ON m.msg_id=i.name_msg_id WHERE i.item_id=? LIMIT 1", (did,))
    name = it[0]['nm'] if it else ("dev%s" % did)
    dskill = it[0]['sk'] if it else 0
    modes = q("SELECT device_mode_id mid, attack_type_value_id atk, damage_type_value_id dmg, deployable_id dep, bot_id bot, device_projectile_id proj FROM asm_data_set_devices_data_set_device_modes WHERE device_id=?", (did,))
    atk = set(); pet = False; proj = False
    for m in modes:
        if m['atk']: atk.add(m['atk'])
        if (m['dep'] or 0) > 0 or (m['bot'] or 0) > 0: pet = True
        if (m['proj'] or 0) > 0: proj = True
    hasradius = False
    if radius_props:
        ph = ",".join(str(p) for p in radius_props)
        hasradius = bool(q("SELECT 1 FROM asm_data_set_device_mode_properties WHERE device_id=? AND prop_id IN (%s) AND base_value>0 LIMIT 1" % ph, (did,)))
    # Classify MELEE FIRST: melee has a cone Effect Radius, which must NOT re-tag it as AOE.
    # Mirrors the server rule in TgDeviceFire__GetEffectGroup.cpp (attack 170/372 = melee, else
    # radius => AOE, else 85/177 = ranged).
    is_melee_dev = bool(atk & {170, 372})
    if is_melee_dev:
        hasradius = False
    cats = set(); heals = False; dmg = False; debuff = False; timedfx = False
    for eg in q("SELECT DISTINCT dme.effect_group_id eg FROM asm_data_set_device_mode_effect_groups dme WHERE dme.device_id=?", (did,)):
        meta = q("SELECT category_value_id cat, lifetime_sec l, required_skill_id rsk FROM asm_data_set_effect_groups WHERE effect_group_id=? LIMIT 1", (eg['eg'],))
        if meta:
            if meta[0]['cat']: cats.add(meta[0]['cat'])
            if (meta[0]['l'] or 0) > 0: timedfx = True
        for e in q("SELECT prop_id p, base_value bv, calc_method_value_id c FROM asm_data_set_effects WHERE effect_group_id=?", (eg['eg'],)):
            pos = e['c'] in (67, 68)
            if e['p'] in (51, 211):
                if pos: heals = True
                else: dmg = True
            if e['p'] in (155, 156, 157, 217, 218, 219, 324, 316) and not pos: debuff = True
    devices[did] = {'name': name, 'skill': dskill, 'class': PROF.get(r['profile_id'], 'Shared'),
                    'atk': sorted(atk), 'pet': pet, 'aoe': hasradius or (proj and 3 in atk), 'proj': proj,
                    'cats': sorted(cats), 'heals': heals, 'dmg': dmg, 'debuff': debuff, 'timedfx': timedfx}

# skill -> devices index
by_skill = {}
for did, d in devices.items():
    by_skill.setdefault(d['skill'], []).append(did)
# category -> devices index
by_cat = {}
for did, d in devices.items():
    for c in d['cats']:
        by_cat.setdefault(c, []).append(did)

print("=== device skill_id coverage ===")
for sk, dl in sorted(by_skill.items()):
    print("  skill %s (%s): %s" % (sk, sname(sk), ", ".join(devices[d]['name'] for d in dl[:14])))

# ---------- 2. tree skills + effect groups ----------
skills = {}
for r in q("""SELECT DISTINCT sgs.skill_group_id grp, sgs.skill_id sid, sgs.name_msg_translated nm
              FROM asm_data_set_skill_group_skills sgs
              WHERE sgs.skill_group_id BETWEEN 155 AND 163 AND sgs.name_msg_translated<>''"""):
    egs = {}
    for s in q("SELECT DISTINCT effect_group_id eg, effect_group_type_value_id t FROM asm_data_set_skill_effect_groups WHERE skill_group_id=? AND skill_id=?", (r['grp'], r['sid'])):
        meta = q("SELECT required_skill_id rsk, required_category_value_id rcat, situational_type_value_id sit, situational_value sv, lifetime_sec l FROM asm_data_set_effect_groups WHERE effect_group_id=? LIMIT 1", (s['eg'],))
        m = meta[0] if meta else None
        effs = [(e['p'], round(e['bv'], 2), e['c'], e['pv'] or 0) for e in q("SELECT prop_id p, base_value bv, calc_method_value_id c, property_value_id pv FROM asm_data_set_effects WHERE effect_group_id=?", (s['eg'],))]
        egs[s['eg']] = {'type': s['t'], 'rsk': m['rsk'] if m else 0, 'rcat': m['rcat'] if m else 0,
                        'sit': m['sit'] if m else 0, 'sv': m['sv'] if m else 0, 'life': m['l'] if m else 0, 'fx': effs}
    if egs:
        skills[(r['grp'], r['sid'])] = {'name': r['nm'], 'tree': TREE[r['grp']], 'cls': TREECLS[r['grp']], 'egs': egs}

print("\n=== tree skills with effect groups: %d ===" % len(skills))

# ---------- 3. resolve interactions ----------
TKIND = {261: 'PASSIVE', 264: 'ON-HIT', 505: 'CONDITIONAL', 759: 'ON-HIT', 1104: 'REACTIVE'}
def fxsum(fx, maxn=3):
    out = []
    for f in fx[:maxn]:
        p, v, c = f[0], f[1], f[2]
        out.append("%s %s%s" % (pn(p), CALC.get(c, ''), v))
    return "; ".join(out)

ix = {}      # device_id -> list of interactions
skill_report = []  # for chat/doc
unresolved = []
for (grp, sid), S in sorted(skills.items()):
    for eg, G in S['egs'].items():
        kind = TKIND.get(G['type'], str(G['type']))
        detail = fxsum(G['fx'])
        gate = ''
        targets = []
        how = ''
        scope = ''
        if G['rsk']:
            targets = by_skill.get(G['rsk'], [])
            how = 'gated to %s' % sname(G['rsk']); scope = 'skill'
            if not targets:
                unresolved.append((S['name'], eg, G['rsk'], sname(G['rsk']), detail))
                continue
        elif G['rcat']:
            targets = by_cat.get(G['rcat'], [])
            how = 'while a category-%s effect is active' % G['rcat']; scope = 'category'
            kind = 'REACTIVE'
            if not targets:
                unresolved.append((S['name'], eg, 'cat%s' % G['rcat'], '', detail))
                continue
        else:
            # ungated: resolve by prop semantics + class compatibility
            props = set(f[0] for f in G['fx'])
            sem = []
            if props & {212}: sem += [d for d, v in devices.items() if set(v['atk']) & {170, 372}]
            if props & {214, 215, 232}: sem += [d for d, v in devices.items() if set(v['atk']) & {85, 177} and (v['dmg'] or v['debuff'])]
            if props & {321, 352}: sem += [d for d, v in devices.items() if v['aoe'] and v['dmg']]
            if props & {350, 381, 382, 383, 366}: sem += [d for d, v in devices.items() if v['pet']]
            if props & {330}: sem += [d for d, v in devices.items() if v['heals']]
            if props & {357, 337}: sem += [d for d, v in devices.items() if devices[d]['skill'] in (365, 351, 363, 364)]
            if not sem:
                continue  # self/defensive stat, no device link
            targets = sorted(set(sem))
            how = 'applies to all matching devices'; scope = 'tree'
        if G['sit'] == 1271: gate = ' (target HP >%s%%)' % int(G['sv'])
        if G['sit'] == 1270: gate = ' (target HP <%s%%)' % int(G['sv'])
        for did in targets:
            # class compatibility: a class-tree skill can only affect that class's devices
            if S['cls'] != 'All' and devices[did]['class'] not in ('Shared', S['cls']):
                continue
            ix.setdefault(did, []).append({'skill': S['name'], 'sid': sid, 'tree': S['tree'], 'kind': kind,
                                           'detail': detail + gate, 'how': how, 'scope': scope,
                                           # numeric backing so the bench can recompute, not just describe
                                           'fx': [list(f) for f in G['fx']],
                                           'egt': G['type'], 'life': G['life'] or 0,
                                           'sit': G['sit'] or 0, 'sv': G['sv'] or 0})
        skill_report.append((S['cls'], S['tree'], S['name'], kind, how, len(targets), detail + gate))

# Unmerged dump for the resolver: ONE entry per effect GROUP, so a skill that has both an
# always-on (261) group and a conditional (505) group keeps them distinct — merging would
# lose the always-on vs situational split the bench depends on.
json.dump({str(k): [dict(e) for e in v] for k, v in ix.items()}, open(OUT + 'ixraw.json', 'w'))
print("ixraw:", sum(len(v) for v in ix.values()), "effect groups")

# Merge per device: ONE entry per skill (a skill can have several effect groups — e.g. Assault
# Melee III has an ungated stat group AND a skill-gated Threat group; it must read as one skill).
KPRI = {'REACTIVE': 0, 'CONDITIONAL': 1, 'ON-HIT': 2, 'PASSIVE': 3}
for did in ix:
    merged = {}
    for e in ix[did]:
        m = merged.get(e['skill'])
        if not m:
            merged[e['skill']] = dict(e, details=[e['detail']], fx=list(e['fx']))
        else:
            if e['detail'] not in m['details']: m['details'].append(e['detail'])
            if KPRI.get(e['kind'], 9) < KPRI.get(m['kind'], 9): m['kind'] = e['kind']
            if e['scope'] == 'skill': m['how'] = e['how']
            for f in e['fx']:
                if f not in m['fx']: m['fx'].append(f)
    out = []
    for m in merged.values():
        m['detail'] = " · ".join(m['details']); m.pop('details')
        out.append(m)
    ix[did] = sorted(out, key=lambda x: (KPRI.get(x['kind'], 9), x['tree'] != 'Balanced', x['skill']))

json.dump({str(k): v for k, v in ix.items()},
          open(r"C:\Users\patri\AppData\Local\Temp\claude\E--GA-LOCAL-Repo\4220e829-c0b4-416e-90e1-0bc04ececb41\scratchpad\ix.json", 'w'), indent=0)

# device classification (attack type / pet / aoe / heals) - the bench needs this to pick the
# right ConvertPropToPropList modifier bucket for a given damage number.
json.dump({str(k): {'name': v['name'], 'cls': v['class'], 'skill': v['skill'], 'atk': v['atk'],
                    'pet': v['pet'], 'aoe': v['aoe'], 'heals': v['heals'], 'dmg': v['dmg']}
           for k, v in devices.items()}, open(OUT + 'devmeta.json', 'w'))
print('devmeta:', len(devices), 'devices')

# skill_id -> the devices it affects (for the character sheet)
skilldev = {}
for did, es in ix.items():
    for e in es:
        if not e.get('sid'): continue
        d = skilldev.setdefault(str(e['sid']), {'name': e['skill'], 'devices': [], 'scope': e['scope']})
        entry = [devices[did]['name'], devices[did]['class']]   # keep class so the sheet can filter
        if entry not in d['devices']: d['devices'].append(entry)
for d in skilldev.values(): d['devices'].sort()
json.dump(skilldev, open(r"C:\Users\patri\AppData\Local\Temp\claude\E--GA-LOCAL-Repo\4220e829-c0b4-416e-90e1-0bc04ececb41\scratchpad\skilldev.json", 'w'))
print("skill->device map:", len(skilldev), "skills")

print("\n=== SKILL -> DEVICE INTERACTION MAP (by class/tree) ===")
cur = None
for cls, tree, name, kind, how, n, detail in sorted(skill_report):
    if (cls, tree) != cur:
        cur = (cls, tree); print("\n-- %s / %s --" % cur)
    print("  %-26s %-11s %-28s ->%3d devices | %s" % (name, kind, how, n, detail[:70]))

print("\n=== UNRESOLVED gates (no inventory device carries the required skill) ===")
for nm, eg, rsk, rn, detail in unresolved:
    print("  %-26s eg%-6s needs %s %s | %s" % (nm, eg, rsk, rn, detail[:60]))

print("\ndevices with interactions:", len(ix), "of", len(devices))
