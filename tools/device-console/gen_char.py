# Real character builds for user 2381 (Jeronix): 4 live characters x 5 item profiles.
#   ga_characters        -> the character (class = profile_id)
#   ga_character_devices -> equipped_slot + inventory_id (the ACTUAL rolled instance)
#   ga_character_skills  -> allocated skill ids per item profile
# Mods come from ga_players_inventory.mod_effect_group_ids on the equipped instance, so each
# piece carries its own real roll rather than a representative variant.
import sqlite3, json, sys
sys.stdout.reconfigure(encoding="utf-8")
OUT = r'C:/Users/patri/AppData/Local/Temp/claude/E--GA-LOCAL-Repo/4220e829-c0b4-416e-90e1-0bc04ececb41/scratchpad/'
db = sqlite3.connect(r"E:\GA_LOCAL\gaa.db"); db.row_factory = sqlite3.Row
def q(s, a=()): return db.execute(s, a).fetchall()

# Accounts to bake in. Inventory is seeded identically for everyone, so these differ in their
# CHARACTERS and in their inventory row IDS - and the ids are what an export writes back with,
# which is why each account carries its own map rather than sharing one.
USERS = ['Jeronix', 'Zipe', 'Kelrior', 'Neophyte', 'Deadly', 'Zaxik', 'Phoron', 'RoundTwo',
         'YeXiuu']
PROF = {680: 'Assault', 567: 'Medic', 681: 'Recon', 679: 'Robotics'}
SLOT = {'221': 'Melee', '198': 'Ranged', '200': 'Specialty', '201': 'Jetpack',
        '203,204,385': 'Offhand', '386': 'Boost', '502': 'Class', '500': 'Consumable'}
ARMSLOT = {1130: 'Head', 1143: 'Shoulder', 1133: 'Chest', 1136: 'Arm',
           1132: 'Hand', 1139: 'Leg', 1142: 'Foot'}
# cosmetic-only slots: dyes, trails, suits - no mechanical effect, hidden from the builder
COSMETIC = {996, 997, 998, 999, 1000, 1001, 202}
# HUMAN BASE ATTRIBUTES is not an equippable choice -- it is on every character and is already
# folded into the base player stats, so it must not appear as a toggleable equipped device.
HBA = 864

# The game calls it Cooldown wherever the player sees it; the property table says Recharge
# Time. Renamed at the point names are produced, so it is consistent across the whole console.
PROPRENAME = {4: 'Cooldown', 203: 'Cooldown Modifier'}
def pn(pid):
    if pid in PROPRENAME: return PROPRENAME[pid]
    r = q("SELECT name FROM asm_data_set_properties WHERE prop_id=?", (pid,))
    return (r[0]['name'] if r and r[0]['name'] else str(pid))
def iname(iid):
    r = q("SELECT m.message FROM asm_data_set_items i JOIN asm_data_set_msg_translations m ON m.msg_id=i.name_msg_id WHERE i.item_id=? LIMIT 1", (iid,))
    return r[0]['message'] if r else None

LET_DEV = {242: 'P', 65: 'D', 214: 'D', 212: 'D', 321: 'D', 350: 'D', 372: 'D', 373: 'D', 374: 'D', 375: 'D',
           330: 'H', 210: 'H', 51: 'H', 211: 'H', 386: 'S', 208: 'T', 355: 'T', 339: 'VN', 366: 'VN',
           352: 'X', 382: 'X', 114: 'R', 381: 'R', 207: 'R', 5: 'R', 153: 'R', 357: 'M',
           10: 'C', 113: 'C', 232: 'F', 376: 'y', 316: 'V', 140: 'K'}
LET_ARM = {412: 'N', 390: 'N', 218: 'R', 217: 'M', 219: 'B', 155: 'P'}
for r in q("SELECT prop_id FROM asm_data_set_properties WHERE name LIKE '%Recharge Time%'"):
    LET_DEV[r['prop_id']] = 'C'

def decode(csv, armor=False):
    """(base_label, group_labels, sig, numeric_totals) for one rolled instance."""
    LET = LET_ARM if armor else LET_DEV
    parts = [t.strip() for t in (csv or '').split(',') if t.strip().isdigit()]
    if not parts: return (None, [], '', [])
    runs = []
    for eg in parts:
        eg = int(eg)
        if runs and runs[-1][0] == eg: runs[-1][1] += 1
        else: runs.append([eg, 1])
    base = None; groups = []; nums = []; sig = ''
    for i, (eg, cnt) in enumerate(runs):
        fx = q("SELECT prop_id p, base_value bv, calc_method_value_id calc FROM asm_data_set_effects WHERE effect_group_id=?", (eg,))
        if not fx: continue
        p, bv, calc = fx[0]['p'], fx[0]['bv'], fx[0]['calc']
        pct = '%' if calc in (68, 69) else ''
        sgn = '+' if calc in (67, 68) else '-'
        nums.append([p, round(bv * cnt, 3), calc, pn(p)])
        if i == 0 and cnt == 1:
            base = "%s %s%s%s" % (pn(p), sgn, round(bv, 2), pct)
        else:
            groups.append("%s %s%s%s  (%dx %s%s%s)" % (pn(p), sgn, round(bv * cnt, 2), pct, cnt, sgn, round(bv, 2), pct))
            sig += LET.get(p, '?') * cnt
    return (base, groups, sig or '\u2014', nums)

def build_chars(USER):
  chars = []
  for c in q("SELECT * FROM ga_characters WHERE user_id=? AND (deleted_at IS NULL OR deleted_at=0) ORDER BY id", (USER,)):
      cls = PROF.get(c['profile_id'])
      if not cls: continue
      profiles = {}
      for pid in range(1, 6):
          devs, arm = [], []
          for r in q("""SELECT cd.equipped_slot slot, pi.id inv, pi.device_id d, pi.oc,
                               pi.allowed_slots asl,
                               pi.mod_effect_group_ids mods, pi.item_id it, pi.quality qual
                        FROM ga_character_devices cd
                        JOIN ga_players_inventory pi ON pi.id=cd.inventory_id
                        WHERE cd.character_id=? AND cd.item_profile_id=? ORDER BY cd.equipped_slot""",
                     (c['id'], pid)):
              slot = r['slot']
              slots = set(int(x) for x in (r['asl'] or '').split(',') if x.strip().isdigit())
              if slot in ARMSLOT:
                  base, groups, sig, nums = decode(r['mods'], armor=True)
                  # slot is the display name the console keys armour by; eslot is the raw
                  # ga_character_devices.equipped_slot a sync has to write back with.
                  arm.append({'slot': ARMSLOT[slot], 'eslot': slot,
                              'inv': r['inv'], 'mods': r['mods'] or '',
                              'name': iname(r['it']) or ARMSLOT[slot],
                              'sig': sig, 'base': base, 'groups': groups, 'nums': nums})
                  continue
              if slots & COSMETIC: continue                   # dyes / trails / suits
              did = r['d'] or 0
              if not did or did == HBA: continue              # cosmetic, or the baked-in HBA
              base, groups, sig, nums = decode(r['mods'])
              # inv + mods are what a sync writes back with: ga_character_devices stores the
              # inventory row id, not a device id, and mods is the authoritative roll on it.
              devs.append({'slot': slot, 'id': did, 'inv': r['inv'], 'mods': r['mods'] or '',
                           'name': iname(did) or ('dev%s' % did),
                           'oc': bool(r['oc']), 'cat': SLOT.get(r['asl'], r['asl']),
                           'sig': sig, 'base': base, 'groups': groups, 'nums': nums})
          skills = [x['skill_id'] for x in
                    q("SELECT skill_id FROM ga_character_skills WHERE character_id=? AND item_profile_id=?",
                      (c['id'], pid))]
          if not devs and not skills and not arm: continue
          profiles[str(pid)] = {'devices': devs, 'armour': arm, 'skills': skills}
      # keep only characters that are actually built out - the empty alts carry nothing but
      # the un-unequippable HUMAN BASE ATTRIBUTES row
      if profiles and any(p['skills'] for p in profiles.values()):
          profiles = {k: v for k, v in profiles.items() if v['skills']}
          chars.append({'id': c['id'], 'cls': cls, 'current': c['current_item_profile_id'], 'profiles': profiles})

  return chars


def build_inv(USER):
  # The whole owned inventory, keyed device -> roll signature -> inventory row.
#
# A profile only shows what is EQUIPPED, which is a small slice of what the account owns (82 of
# 270 rows here). Since inventory is append-only - you cannot re-mod, acquire or destroy - these
# rows are the complete and permanent set of items this account can ever express, so an export
# can resolve any build made from them, not just one that happens to be equipped already.
  inv = {}
  for r in q("""SELECT id, device_id, mod_effect_group_ids mods, allowed_slots asl
                FROM ga_players_inventory WHERE user_id=? AND device_id>0""", (USER,)):
    did = r['device_id']
    if did == HBA:
        continue
    slots = set(int(x) for x in (r['asl'] or '').split(',') if x.strip().isdigit())
    if slots & COSMETIC:
        continue
    sig = decode(r['mods'])[2]
    inv.setdefault(str(did), {}).setdefault(sig, [r['id'], r['mods'] or ''])
  return inv


accounts = []
for uname in USERS:
    row = q("SELECT id, username FROM ga_users WHERE username=? COLLATE NOCASE", (uname,))
    if not row:
        print("  !! no such user:", uname)
        continue
    uid, real = row[0]['id'], row[0]['username']
    chars = build_chars(uid)
    if not chars:
        print("  -- %s (%s) has no built-out characters, skipped" % (real, uid))
        continue
    inv = build_inv(uid)
    accounts.append({'user': uid, 'name': real, 'chars': chars, 'inv': inv})
    print("  %-10s id=%-6s chars=%s  inv=%d devices" % (real, uid, len(chars), len(inv)))
    for c in chars:
        print("      %-5s %-9s profiles=%s" % (c['id'], c['cls'], sorted(c['profiles'].keys())))

json.dump({'accounts': accounts, 'active': accounts[0]['user'] if accounts else None},
          open(OUT + 'chars.json', 'w'))
print("accounts:", len(accounts))
