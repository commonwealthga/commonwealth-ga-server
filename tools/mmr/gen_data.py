"""Build standings.json + spotlight.json from a reseeded database.

Archetype shown is the player's LATEST, not their most common: it is what they
are now and what their next match will be scored as. Early games are labelled
before the class baselines have settled, so a mode over all history can report
someone as what they used to look like.
"""
import sqlite3, math, json, statistics as st, sys
from collections import defaultdict
import reseed
from reseed import TUN, STATS

DB = sys.argv[1] if len(sys.argv) > 1 else "E:/server-redacted.db"
URI = "file:" + DB + "?mode=ro"
c = sqlite3.connect(URI, uri=True)
q = lambda s: c.execute(s).fetchall()
col = next(x[1] for x in q("PRAGMA table_info(ga_users)")
           if x[1] in ("username", "name", "display_name"))
NM = {u: n for u, n in q(f"SELECT id,{col} FROM ga_users")}

cur, gm, arch_last, pf, wl_, ex_ = {}, defaultdict(int), {}, \
    defaultdict(list), defaultdict(list), defaultdict(list)
for u, cl, a, p, e, act, after, g in q("""SELECT user_id,class_name,archetype,perf,expected,
        actual,mmr_after,games_after FROM ga_mmr_history ORDER BY instance_id"""):
    k = (u, cl)
    cur[k] = after; gm[k] = g; arch_last[k] = a
    pf[k].append(p); wl_[k].append(act); ex_[k].append(e)

oldwl = {}
try:
    for u, cl, v in q("SELECT user_id,class_name,mmr_after FROM ga_wl_mmr_history_oldengine ORDER BY rowid"):
        oldwl[(u, cl)] = v
except sqlite3.OperationalError:
    pass


def settle(r0, perf, surprise):
    """The team term does not depend on the player's rating, so the resting
    point is r = 1000 + scale*(perf + surprise/beta). Simulated for exactness."""
    r, n = r0, 0
    while n < 5000:
        d = TUN["k_base"] * (surprise + TUN["beta"] *
                             (perf - (r - TUN["default_mmr"]) / TUN["perf_scale"]))
        r += d; n += 1
        if abs(d) < 0.25:
            break
    return round(r), n


MIN = {"Assault": 20, "Recon": 20, "Medic": 20, "Robotic": 10}
out = {}
for cls in ("Assault", "Recon", "Medic", "Robotic"):
    rows = []
    for k, v in cur.items():
        if k[1] != cls or gm[k] < 10:
            continue
        mp = st.mean(pf[k])
        wr = sum(1 for x in wl_[k] if x == 1.0) / len(wl_[k])
        proj, ng = settle(v, mp, wr - st.mean(ex_[k]))
        rows.append(dict(name=(NM.get(k[0]) or f"u{k[0]}")[:18], mmr=round(v),
                         games=gm[k], arch=arch_last[k], perf=round(mp, 2),
                         win=round(wr * 100),
                         old=(round(oldwl[k]) if k in oldwl else None),
                         proj=proj, proj_games=gm[k] + ng))
    rows.sort(key=lambda r: -r["mmr"])
    out[cls] = rows
json.dump(out, open("standings.json", "w"))

# --- per-stat profiles for the spotlight tables -----------------------------
matches = reseed.load(c)
pop, per = defaultdict(list), defaultdict(list)
for m in matches:
    if len(m["players"]) < TUN["min_rated_players"]:
        continue
    for p in m["players"]:
        pop[p["cls"]].append(p["rate"])
        per[(p["uid"], p["cls"])].append(p["rate"])
base = {}
for cls, rows in pop.items():
    b = []
    for k in range(len(STATS)):
        v = [r[k] for r in rows]
        mu = sum(v) / len(v)
        b.append((mu, math.sqrt(max(0.0, sum(x * x for x in v) / len(v) - mu * mu))))
    base[cls] = b

lookup = {(cls, r["name"]): r for cls, rows in out.items() for r in rows}
rev = defaultdict(list)
for u, n in NM.items():
    if n:
        rev[n.lower()].append(u)
SHOW = ["kills", "assists", "deaths", "damage_dealt", "damage_taken",
        "healing", "defense", "buff_value", "obj_points", "rel_deaths"]
PICK = {"Assault": ["Zipe", "AngelDeLaGuarda", "Kelrior"],
        "Recon":   ["Kelrior", "donk", "Marksy"],
        "Medic":   ["Compelling", "Kloudnine", "amna", "Callizle", "RoundTwo"],
        "Robotic": ["Jeronix", "Zaxik", "WaRadius"]}
sp = {}
for cls, names in PICK.items():
    sp[cls] = []
    for nm in names:
        u = next((x for x in rev[nm.lower()] if (x, cls) in per), None)
        row = lookup.get((cls, nm))
        if u is None or row is None:
            print("  missing:", nm, cls)
            continue
        rows = per[(u, cls)]
        e = {}
        for k, s in enumerate(STATS):
            if s not in SHOW:
                continue
            mu, sd = base[cls][k]
            m = sum(r[k] for r in rows) / len(rows)
            e[s] = dict(val=round(m, 2), z=round((m - mu) / sd if sd else 0.0, 2))
        sp[cls].append(dict(row, stats=e))
json.dump(sp, open("spotlight.json", "w"))

allrows = [dict(r, cls=cl) for cl, v in out.items() for r in v]
for r in allrows:
    r["d"] = r["proj"] - r["mmr"]
d = [abs(r["d"]) for r in allrows]
print(f"{len(allrows)} rated. median |proj-now| {st.median(d):.0f}, "
      f"within 50: {sum(1 for x in d if x <= 50)}, over 100: {sum(1 for x in d if x > 100)}")
counts = defaultdict(int)
for r in allrows:
    counts[r["arch"]] += 1
print("archetypes (current label):", dict(counts))
for cls in ("Assault", "Recon", "Medic", "Robotic"):
    print(f"  {cls}: " + ", ".join(f"{r['name']} {r['mmr']}({r['arch']})" for r in out[cls][:4]))
