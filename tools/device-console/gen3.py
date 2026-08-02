import os
# Render inventory console v2 from inv_model.json
import json, html, os
SEP = os.sep
BASE = r"C:\Users\patri\AppData\Local\Temp\claude\E--GA-LOCAL-Repo\4220e829-c0b4-416e-90e1-0bc04ececb41\scratchpad"
D = json.load(open(BASE + r"\inv_model.json"))
DEVIMG = open(BASE + SEP + 'deviceimg.json', encoding='utf-8').read()
IMG = json.loads(DEVIMG)
IXD = json.load(open(BASE + r"\ix.json"))
model = D['model']; armor = D['armor']; base = D['base']
ORDER = ['Assault', 'Medic', 'Recon', 'Robotics']
ACC = {'Assault': 'assault', 'Medic': 'medic', 'Recon': 'recon', 'Robotics': 'robotics'}
# ---- class icons (Assault/Medic/Recon/Robotics + death) ---------------------------
# Same trick as the device art: base64 once into a lookup, referenced by name at runtime, so the
# page stays a single self-contained file with no external requests.
import base64 as _b64
OUTDIR = os.path.join('E:' + os.sep, 'GA_LOCAL', 'Repo', 'docs', 'claude',
                      'theorycraft-console')
CLASSIMG = {}
_cdir = OUTDIR + SEP + 'assets' + SEP + 'class-icons'
if os.path.isdir(_cdir):
    for _f in sorted(os.listdir(_cdir)):
        if not _f.lower().endswith('.png'):
            continue
        with open(os.path.join(_cdir, _f), 'rb') as _fh:
            CLASSIMG[os.path.splitext(_f)[0]] = ('data:image/png;base64,'
                                                 + _b64.b64encode(_fh.read()).decode('ascii'))
CLASSIMGJSON = json.dumps(CLASSIMG)

CATORD = ['Melee', 'Ranged', 'Specialty', 'Offhand', 'Boost', 'Jetpack']
def esc(s): return html.escape(str(s))
CHIP = {'heal': 'heal', 'prot': 'prot', 'dmg': 'dmg', 'debuff': 'debuff', 'power': 'power', 'hp': 'hp', 'util': 'util', 'blk': 'blk', 'mech': 'mech', 'stat': 'stat'}
# interaction kind -> css + label (data-driven from ix.json)
KIND = {'REACTIVE': 'k-react', 'CONDITIONAL': 'k-cond', 'PASSIVE': 'k-amp', 'ON-HIT': 'k-hit'}
DEVIX = {}
def ix_rows(did):
    """Interaction chips grouped by the three trees that can affect a device:
       Balanced (all classes) + the class's two specialisation trees."""
    entries = IXD.get(str(did), [])
    if not entries: return ''
    DEVIX[str(did)] = [[e.get('sid') or 0, e['kind'], e['skill'], e['detail']] for e in entries if e.get('sid')]
    bytree = {}
    for e in entries:
        bytree.setdefault(e['tree'], []).append(e)
    order = ['Balanced'] + sorted(t for t in bytree if t != 'Balanced')
    rows = []
    for tree in order:
        if tree not in bytree: continue
        chips = "".join(
            '<span class="ix %s" title="%s &mdash; %s"><em>%s</em> %s</span>'
            % (KIND.get(e['kind'], 'k-amp'), esc(e['detail']), esc(e['how']), esc(e['kind']), esc(e['skill']))
            for e in bytree[tree])
        rows.append('<div class="ixtree"><span class="treelab">%s</span><div class="ixchips">%s</div></div>' % (esc(tree), chips))
    return '<div class="ixrow">%s</div>' % "".join(rows)
def chiprow(chips):
    out = []
    for c in chips:
        k, t = c[0], c[1]
        cls = CHIP.get(k, 'util')
        if t.startswith('Mech: '):
            out.append('<span class="chip %s selfx"><i class="who mech">vs mech</i>%s</span>' % (cls, esc(t[6:])))
        elif t.startswith('Self: '):
            out.append('<span class="chip %s selfx"><i class="who">self</i>%s</span>' % (cls, esc(t[6:])))
        elif t.startswith('Counter: '):
            out.append('<span class="chip %s selfx"><i class="who atk">atk</i>%s</span>' % (cls, esc(t[9:])))
        elif t.startswith('Backstab: '):
            out.append('<span class="chip %s"><i class="who bs">backstab</i>%s</span>' % (cls, esc(t[10:])))
        elif t.startswith('Zoom: '):
            out.append('<span class="chip %s"><i class="who">zoom</i>%s</span>' % (cls, esc(t[6:])))
        else:
            out.append('<span class="chip %s">%s</span>' % (cls, esc(t)))
    return "".join(out)
def pwchip(p):
    return '<span class="chip pw">%s pwr</span>' % (int(p) if p == int(p) else p) if p is not None else ''
def sigchip(v):
    tip = (("base " + v['base'] + " · ") if v.get('base') else "") + " · ".join(v['groups'])
    return '<span class="sig" title="%s">%s</span>' % (esc(tip), esc(v['sig']))

MTAG = {'PRI': ('pri', 'PRIMARY'), 'ALT': ('alt', 'ALT'), 'BLOCK': ('blkt', 'ALT&nbsp;&middot;&nbsp;BLOCK'), 'SPAWN': ('spawn', 'ON&nbsp;IMPACT')}
def card(d):
    oc = '<span class="oc">OC</span>' if d['oc'] else ''
    modes = d['modes'] or [{'kind': 'PRI', 'name': '', 'power': None, 'chips': []}]
    rows = []
    for m in modes:
        cls, lbl = MTAG.get(m.get('kind', 'PRI'), ('pri', 'PRIMARY'))
        nm = ('<span class="mname">%s</span>' % esc(m['name'])) if m.get('name') else ''
        rows.append('<div class="mrow"><span class="mtag %s">%s</span><div class="chips">%s%s%s</div></div>'
                    % (cls, lbl, nm, chiprow(m['chips']), pwchip(m['power'])))
    body = '<div class="modes">%s</div>' % "".join(rows)
    seen = set(); sigs = []
    for v in d['variants']:
        if v['sig'] in seen: continue
        seen.add(v['sig']); sigs.append(sigchip(v))
    sigrow = ('<div class="sigs"><span class="siglab">MOD</span>%s</div>' % "".join(sigs)) if sigs else ''
    did = d.get('id', 0)
    # Text interaction chips. The per-card mini skill trees are disabled: ~4400 absolutely
    # positioned nodes with inline base64 icons made browsing the loadout noticeably slow.
    ixhtml = ix_rows(did)
    # src is filled at runtime from window.__DEVIMG__ so the base64 ships once, not twice
    img = ('<img class="devimg" data-i="%s" alt="" loading="lazy">' % did) if str(did) in IMG else ''
    return ('<article class="card" data-dev="%s"><div class="chead">%s<h4>%s</h4>%s</div>%s%s%s</article>'
            % (did, img, esc(d['name']), oc, body, sigrow, ixhtml))

def cls_pane(cls):
    cats = model[cls]
    order = [c for c in CATORD if c in cats] + [c for c in cats if c not in CATORD]
    blocks = []
    for cat in order:
        cards = "".join(card(d) for d in sorted(cats[cat], key=lambda x: x['name']))
        blocks.append('<section class="catblock"><div class="cathead"><h3>%s</h3><span class="cnt">%d</span></div><div class="grid">%s</div></section>' % (esc(cat), len(cats[cat]), cards))
    return '<div class="classpane %s" data-class="%s"><div class="clshead"><span class="dot"></span><h2>%s</h2><span class="clssub">%d devices · live loadout</span></div>%s</div>' % (
        ACC[cls], cls, cls.upper(), sum(len(v) for v in cats.values()), "".join(blocks))

# Base + Armor pane
base_card = '<article class="card"><div class="chead"><h4>Human Base Attributes</h4><span class="oc" style="background:var(--gear)">DEFAULT</span></div><div class="chips">%s</div><div class="sigs"><span class="siglab">REST</span><div class="chips">%s</div></div></article>' % (chiprow(base['equip']), chiprow(base['rest']))
ASORD = ['Head', 'Shoulder', 'Chest', 'Arm', 'Hand', 'Leg', 'Foot']
acards = [base_card]
for slot in ASORD:
    if slot not in armor: continue
    sigs = "".join('<span class="sig" title="%s">%s</span>' % (esc(" · ".join(v['groups'])), esc(v['sig'])) for v in armor[slot])
    acards.append('<article class="card"><div class="chead"><h4>%s Armor</h4></div><div class="chips"><span class="chip hp">+10%% Max HP base</span></div><div class="sigs"><span class="siglab">MOD</span>%s</div></article>' % (esc(slot), sigs))
armor_pane = '<div class="classpane gear" data-class="Gear"><div class="clshead"><span class="dot"></span><h2>BASE &amp; ARMOR</h2><span class="clssub">shared by all classes · HBA can\'t be unequipped · armor mods: R/M/A ranged/melee/aoe prot · N +HP</span></div><section class="catblock"><div class="grid">%s</div></section></div>' % "".join(acards)

panes = "".join(cls_pane(c) for c in ORDER) + armor_pane
tabs = "".join('<button class="tab %s" data-tab="%s">%s</button>' % (ACC[c], c, c) for c in ORDER) + '<button class="tab gear" data-tab="Gear">Base &amp; Armor</button>'
legend = ''.join('<b><span class="sw" style="background:var(--%s)"></span>%s</b>' % (v, k) for k, v in
    [('heal', 'heal'), ('dmg', 'damage'), ('prot', 'protect'), ('debuff', 'debuff'), ('power', 'power')])
klegend = ''.join('<b><span class="sw" style="background:var(--%s)"></span>%s</b>' % (v, k) for k, v in
    [('k-react', 'reactive'), ('k-cond', 'conditional'), ('k-amp', 'passive-scoped'), ('k-hit', 'on-hit'), ('k-glob', 'global')])
STYLE = open(BASE + r"\style2.css", encoding="utf-8").read()
SCRIPT = open(BASE + r"\app.js", encoding="utf-8").read()
BUILDJS = open(BASE + r"\builder.js", encoding="utf-8").read()
TREEJSON = open(BASE + r"\tree.json", encoding="utf-8").read()
BENCHJS = open(BASE + SEP + 'bench.js', encoding='utf-8').read()
DEVMETA = json.load(open(BASE + SEP + 'devmeta.json'))
CHARS = open(BASE + SEP + 'chars.json', encoding='utf-8').read()
# gate-resolved interactions with their NUMERIC effects - what GA.resolve consumes
IXRAW = json.load(open(BASE + SEP + 'ixraw.json'))
DEVFX = {k: [{'sid': e['sid'], 'skill': e['skill'], 'tree': e['tree'], 'kind': e['kind'],
              'detail': e['detail'], 'how': e['how'], 'egt': e['egt'], 'fx': e['fx']}
             for e in v if e.get('sid')] for k, v in IXRAW.items()}
DEVMODEL = {}
for _cls, _cats in model.items():
    for _cat, _ds in _cats.items():
        for _d in _ds:
            DEVMODEL[str(_d['id'])] = {'name': _d['name'], 'cat': _d['cat'], 'oc': _d['oc'],
                                       'modes': _d['modes'], 'variants': _d['variants']}
CLSBTN = "".join('<button class="tb-class%s" data-cls="%s"><img class="clsic sm" src="%s" alt="">%s</button>'
                 % (' active' if c == 'Assault' else '', c, CLASSIMG.get(c, ''), c) for c in ORDER)
BUILDER = (
 '<div id="tb-char"></div>'
 '<div class="tbwrap" id="tbwrap"><div id="tb-gear"></div><div id="tb-armour"></div><div id="tb-trees"></div>'
 '<aside id="tb-sheet"></aside></div>')
# The class selector + point counter belong to the scratch build: on My Character the loaded
# profile decides the class, so a manual picker there is misleading.
CRAFTBAR = (
 '<div class="tbbar">' + CLSBTN +
 '<span id="tb-msg"></span>'
 '<div class="ptbox"><span id="tb-count">0 / 13</span><button id="tb-reset">Reset</button></div></div>'
 '<p class="intro">Pick a class, fill the equipment slots, then spend skill points. '
 'The same rules as the game apply: <strong>13 points</strong> total, each skill costs 1, a node needs its '
 '<strong>tier requirement</strong> (points already spent in that tree) and its <strong>prerequisite skill</strong>. '
 'Click a node to spend, <strong>shift-click or right-click to refund</strong> &mdash; refunding cascades. '
 'Nothing here is written back to the database.</p>'
 '<div id="tc-build"></div>')
# ---- Combat tab: two saved builds, one shot resolved through CalcProtection ----------
COMBATBAR = (
 '<div class="cbtwrap">'
 '<div class="chhead"><h3>Combat</h3><span class="chname">attacker vs defender</span></div>'
 '<p class="chnote">Pick a build for each side. The attacker&rsquo;s weapons are resolved with that '
 'build&rsquo;s own skills and switched-on gear, then run through the defender&rsquo;s protections '
 'the way the game does it &mdash; three axes, each divided by the weapon&rsquo;s attack rating, '
 'multiplied together. Figures are rounded the way the combat log rounds them, so they can be '
 'compared against a real fight directly.</p>'
 '<div id="cb-body"></div></div>')


HTML = (
'<title>Global Agenda &mdash; Loadout & Device Console</title>\n<style>%s</style>\n'
'<header class="top"><div class="topinner">'
'<div class="brand"><h1>Loadout Console</h1><span class="tag">Global Agenda</span>'
'<span class="sub">every valid device &amp; armor piece, per class &mdash; live level-50 inventory</span></div>'
'<div class="viewtabs"><button class="viewtab active" data-view="loadout">Loadout</button>'
'<button class="viewtab" data-view="tree">My Character</button>'
'<button class="viewtab" data-view="craft">TheoryCrafter</button>'
'<button class="viewtab" data-view="combat">Combat</button></div>'
'<div id="loadout-nav"><div class="tabs"><button class="tab all active" data-tab="All">All</button>%s</div>'
'<div class="legend">%s<span style="opacity:.35">&#9474;</span>%s<span style="opacity:.35">&#9474;</span>'
'<b><span class="ocdot">OC</span>overcharged</b> <b><span class="sigdot">AaN</span>mod &mdash; hover</b> <b><span class="mtag pri" style="font-size:9px">PRI</span>/<span class="mtag alt" style="font-size:9px">ALT</span> fire mode</b></div>'
'</div></div></header>\n<div class="wrap">'
'<div id="view-loadout">'
'<p class="intro">The definitive loadout &mdash; from the live level-50 inventory (shared by all since the rebuild removed looting), so only <strong>truly-usable</strong> devices appear. Cards show <strong>calc-accurate effects</strong> per input, <strong>OC</strong> status, <strong>mod-signature variants</strong> (hover for exact modifiers), and interacting skills. Data layer for the combat simulator.</p>'
'<div class="panel"><h5>How to read a card</h5><div class="pgrid">'
'<div><span class="mtag pri">PRIMARY</span><p>Left click. The device&rsquo;s main attack / heal / deploy.</p></div>'
'<div><span class="mtag alt">ALT</span><p>Right click. A second fire mode &mdash; heavy attack, alt-fire, or aim/zoom (ADS).</p></div>'
'<div><span class="mtag blkt">ALT&nbsp;&middot;&nbsp;BLOCK</span><p>Right click <em>holds a block</em> instead of firing. Negates melee damage from the front and drains power per second. Some weapons also counter the attacker.</p></div>'
'<div><span class="chip debuff">Slow -25%%</span><p><b>Solid chip</b> = lands on <em>whatever you hit</em> (the target).</p></div>'
'<div><span class="chip debuff selfx"><i class="who">self</i>Slow -35%%</span><p><b>Hollow + dashed</b> = lands on <em>you</em> &mdash; while equipping, firing, cooling down or aiming.</p></div>'
'<div><span class="sig">DDPPP</span><p>Mod signature &mdash; hover it for the exact modifiers. Each letter is one mod; the game shows the <em>group total</em>.</p></div>'
'</div></div>'
'%s'
'</div>'                                  # /#view-loadout
'<div id="view-tree" style="display:none">@@BUILDER@@</div>'
'<div id="view-craft" style="display:none">@@CRAFT@@</div>'
'<div id="view-combat" class="widewrap" style="display:none">@@COMBAT@@</div>'
'<footer>Source: ga_players_inventory (user 2381) + asm effects (calc-applied) + decompiled UC. Jetpacks: only Crescent used. '
'<b>Device mod letters:</b> P power · D damage · H heal · R range · C cooldown · X AOE radius · T duration · S shield · VN deployable/pet HP · M morale. '
'<b>Armor:</b> N health · R ranged prot · M melee prot · B AOE prot. Each mod group shows the <em>game-displayed total</em> (hover for the per-mod maths). '
'<b>Damage chip = raw device base.</b> The in-game tooltip number is base x (1 + Output Mod) &mdash; e.g. Brawler&rsquo;s 280 x 1.70 = 476, Ballista 585 x 1.75 = 1023. '
'Backstab = type-505 groups w/ situational 509 (UC: GetSituationalMeleeEffectToApply -> 509 backstab / 718 block-breaker). '
'<b>Who an effect lands on</b> is set by the effect-group type (TgDevice.uc constants + call sites): SELF = 261 EQUIP / 262 BUILDUP / 263 FIRE / 265 COOLDOWN / 266 AIM / 283 EQUIP_MODE / 759 SUCCESSFUL_HIT / 1104 REACTIVE (all <code>ApplyEffectType(Instigator)</code>); '
'TARGET = 264 HIT / 272 HIT_IN_AIR / 505 HIT_SITUATIONAL (<code>SubmitHitEffects &rarr; Impact.HitActor</code>); ATTACKER = 398 BLOCK_HIT. '
'<b>Melee has exactly two inputs.</b> <code>right_click_behavior_type_value_id</code> decides RMB: <b>894</b> = hold to <b>Block</b> (costs prop-322 power/sec; blocked melee deals no damage), '
'<b>830</b> = RMB is an <b>alt-fire</b> instead, so the weapon cannot block (only Brawler&rsquo;s Beat Stick &amp; Assassin Blade &mdash; their type-398 data is unreachable and is hidden here). '
'<b>Block counter</b> = type-398 dealt to the attacker; block-only (Dual Daggers, Life Stealer, Impact Hammer) vs block+counter (rest; Mace &amp; Shield hits hardest at 360 and blocks for 1/s vs the usual 15/s). '
'&ldquo;vs Mech&rdquo; = prop 388, damage vs MECHANICAL-physicality targets (turrets/drones/deployables) only &mdash; not a blanket bonus.</footer>'
'</div>\n<script>%s</script>\n') % (STYLE, tabs, legend, klegend, panes, SCRIPT)
# inject builder markup + data + logic without %-formatting (JSON/JS contain % chars)
HTML = HTML.replace('@@BUILDER@@', BUILDER)
HTML = HTML.replace('@@CRAFT@@', CRAFTBAR)
HTML = HTML.replace('@@COMBAT@@', COMBATBAR)
HTML = HTML.replace('</script>\n',
                    '</script>\n<script>window.__TREE__=' + TREEJSON + ';</script>\n'
                    '<script>window.__DEVMODEL__=' + json.dumps(DEVMODEL) + ';</script>\n'
                    '<script>window.__DEVMETA__=' + json.dumps(DEVMETA) + ';</script>\n'
                    '<script>window.__DEVFX__=' + json.dumps(DEVFX) + ';</script>\n'
                    '<script>window.__DEVIMG__=' + DEVIMG + ';</script>\n'
                    '<script>window.__CLASSIMG__=' + CLASSIMGJSON + ';</script>\n'
                    # __CHARS__ stays the ACTIVE account, so every existing reader is unchanged
                    '<script>window.__ACCTS__=' + CHARS + ';'
                    'window.__CHARS__=(window.__ACCTS__.accounts||[])[0]||null;</script>\n'
                    '<script>' + BUILDJS + '</script>\n'
                    '<script>' + BENCHJS + '</script>\n', 1)
open(r"E:\GA_LOCAL\Repo\docs\claude\theorycraft-console\device-console.html", "w", encoding="utf-8").write(HTML)
print("wrote", len(HTML), "bytes")
