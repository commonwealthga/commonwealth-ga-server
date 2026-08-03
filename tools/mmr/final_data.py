"""Gather everything the final report needs: multi-class player profiles, and
past steamrolls with what the balancer would have done to them."""
import sqlite3, math, json, itertools, statistics as st
from collections import defaultdict
import reseed
from reseed import TUN, STATS, IX

DB = "file:E:/server-redacted.db?mode=ro"
conn = sqlite3.connect(DB, uri=True)
col = next(x[1] for x in conn.execute("PRAGMA table_info(ga_users)")
           if x[1] in ("username", "name", "display_name"))
NM = {u: n for u, n in conn.execute(f"SELECT id,{col} FROM ga_users")}
rev = defaultdict(list)
for u, n in NM.items():
    if n:
        rev[n.lower()].append(u)

matches = reseed.load(conn)
rated = [m for m in matches if len(m["players"]) >= TUN["min_rated_players"]]

# class populations for z-scores
pop, per = defaultdict(list), defaultdict(list)
for m in rated:
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

# current ratings from the reseeded history
cur, gm, arch_last, pf, wl_, ex_ = {}, defaultdict(int), {}, \
    defaultdict(list), defaultdict(list), defaultdict(list)
for u, cl, a, p, e, act, after, g in conn.execute(
        """SELECT user_id,class_name,archetype,perf,expected,actual,mmr_after,games_after
           FROM ga_mmr_history ORDER BY instance_id"""):
    k = (u, cl)
    cur[k] = after; gm[k] = g; arch_last[k] = a
    pf[k].append(p); wl_[k].append(act); ex_[k].append(e)

oldwl = {}
for u, cl, v in conn.execute(
        "SELECT user_id,class_name,mmr_after FROM ga_wl_mmr_history_oldengine ORDER BY rowid"):
    oldwl[(u, cl)] = v


def settle(r0, perf, surprise):
    r, n = r0, 0
    while n < 5000:
        d = TUN["k_base"] * (surprise + TUN["beta"] *
                             (perf - (r - TUN["default_mmr"]) / TUN["perf_scale"]))
        r += d; n += 1
        if abs(d) < 0.25:
            break
    return round(r)


SHOW = ["kills", "assists", "deaths", "damage_dealt", "damage_taken",
        "healing", "defense", "buff_value", "obj_points", "rel_deaths"]
NAMES = ["Jeronix", "Zipe", "Kelrior", "Peltz", "Zaxik", "amna", "Callizle"]
players = {}
for nm in NAMES:
    entries = []
    for cls in ("Assault", "Recon", "Medic", "Robotic"):
        u = next((x for x in rev[nm.lower()] if (x, cls) in cur), None)
        if u is None or gm[(u, cls)] < 5:
            continue
        k = (u, cls)
        rows = per[k]
        z = {}
        for i, s in enumerate(STATS):
            if s not in SHOW:
                continue
            mu, sd = base[cls][i]
            m = sum(r[i] for r in rows) / len(rows)
            z[s] = round((m - mu) / sd if sd else 0.0, 2)
        wr = sum(1 for x in wl_[k] if x == 1.0) / len(wl_[k])
        entries.append(dict(cls=cls, mmr=round(cur[k]), games=gm[k],
                            arch=arch_last[k], perf=round(st.mean(pf[k]), 2),
                            win=round(wr * 100),
                            old=(round(oldwl[k]) if k in oldwl else None),
                            proj=settle(cur[k], st.mean(pf[k]), wr - st.mean(ex_[k])),
                            z=z))
    entries.sort(key=lambda e: -e["games"])
    if entries:
        players[nm] = entries
json.dump(players, open("final_players.json", "w"))
print("player profiles:")
for nm, e in players.items():
    print(f"  {nm:<10} " + "  ".join(
        f"{x['cls'][:3]} {x['mmr']}({x['games']}g,{x['arch'][:4]})" for x in e))

# ---- steamrolls, and what the balancer would have done --------------------
CLASSES = ("Assault", "Recon", "Medic", "Robotic")
GAPELO, LAM = TUN["elo_per_player_gap"], 0.5


def winprob(P, asg, tfs):
    a = [p for p in P if asg[p["uid"]] == tfs[0]]
    b = [p for p in P if asg[p["uid"]] == tfs[1]]
    if not a or not b:
        return None
    aa = sum(p["mmr"] for p in a) / len(a)
    ab = sum(p["mmr"] for p in b) / len(b)
    g = min(abs(len(a) - len(b)), 3)
    e1, e2 = aa, ab
    if len(a) > len(b):
        e1 += g * GAPELO / 2; e2 -= g * GAPELO / 2
    elif len(b) > len(a):
        e2 += g * GAPELO / 2; e1 -= g * GAPELO / 2
    cg = {}
    for cl in CLASSES:
        x = [p["mmr"] for p in a if p["cls"] == cl]
        y = [p["mmr"] for p in b if p["cls"] == cl]
        cg[cl] = ((sum(x) / len(x) if x else 0), (sum(y) / len(y) if y else 0), len(x), len(y))
    return dict(a1=aa, a2=ab, n1=len(a), n2=len(b),
                p=1.0 / (1.0 + 10.0 ** ((e2 - e1) / TUN["elo_divisor"])), cls=cg)


scores = {}
for iid, tf, k, d, o in conn.execute(
        """SELECT instance_id,task_force,SUM(kills),SUM(deaths),SUM(obj_points)
           FROM ga_match_player_stats GROUP BY 1,2"""):
    scores.setdefault(iid, {})[tf] = (k or 0, d or 0, o or 0)

cands = []
for m in rated:
    tfs = sorted({p["tf"] for p in m["players"]})
    if len(tfs) != 2 or m["wtf"] == 0:
        continue
    P = [dict(uid=p["uid"], cls=p["cls"], mmr=cur.get((p["uid"], p["cls"]), 1000.0),
              name=(NM.get(p["uid"]) or "?")[:16]) for p in m["players"]]
    actual = {p["uid"]: pp["tf"] for p, pp in zip(P, m["players"])}
    for p, pp in zip(P, m["players"]):
        p["tf"] = pp["tf"]
    w = winprob(P, actual, tfs)
    if w is None:
        continue
    s = scores.get(m["id"], {})
    kw = s.get(m["wtf"], (0, 0, 0))[0]
    kl = s.get(tfs[1] if m["wtf"] == tfs[0] else tfs[0], (0, 0, 0))[0]
    if kl >= 8 and kw / max(kl, 1) < 3.0:
        continue                      # not a steamroll
    fav = w["p"] if m["wtf"] == tfs[0] else 1 - w["p"]
    cands.append((kw / max(kl, 1), m, P, actual, tfs, w, kw, kl, fav))

cands.sort(key=lambda x: -x[0])
out = []
for ratio, m, P, actual, tfs, w, kw, kl, fav in cands[:6]:
    byc = {cl: [p for p in P if p["cls"] == cl] for cl in CLASSES}
    n1 = {cl: sum(1 for p in byc[cl] if actual[p["uid"]] == tfs[0]) for cl in CLASSES}
    tot = {cl: len(byc[cl]) for cl in CLASSES}
    sz1 = sum(1 for p in P if actual[p["uid"]] == tfs[0])
    allowed = {cl: sorted({tot[cl] // 2, (tot[cl] + 1) // 2}) for cl in CLASSES}
    combos = [c for c in itertools.product(*[allowed[cl] for cl in CLASSES]) if sum(c) == sz1]
    if not combos:
        combos = [tuple(n1[cl] for cl in CLASSES)]
    best, base_off = None, abs(w["p"] - 0.5)
    for counts in combos:
        opts = [[set(x) for x in itertools.combinations([p["uid"] for p in byc[cl]], n)]
                if byc[cl] else [set()] for cl, n in zip(CLASSES, counts)]
        for combo in itertools.product(*opts):
            sel = set().union(*combo) if combo else set()
            asg = {p["uid"]: (tfs[0] if p["uid"] in sel else tfs[1]) for p in P}
            r = winprob(P, asg, tfs)
            if r is None or abs(r["p"] - 0.5) > base_off + 1e-9:
                continue
            val = abs(r["p"] - 0.5) * 400 + LAM * sum(
                abs(x - y) for x, y, c1, c2 in r["cls"].values() if c1 and c2)
            nmv = sum(1 for p in P if actual[p["uid"]] != asg[p["uid"]])
            if best is None or (round(val, 6), nmv) < (round(best[0], 6), best[1]):
                best = (val, nmv, asg, r)
    if best is None:
        continue
    mp, = conn.execute("SELECT map_name FROM ga_instances WHERE id=?", (m["id"],)).fetchone()
    out.append(dict(iid=m["id"], map=mp, kills=[kw, kl], won=m["wtf"],
                    before=dict(p=round(fav * 100, 1), n1=w["n1"], n2=w["n2"],
                                a1=round(w["a1"]), a2=round(w["a2"]),
                                cls={k2: [round(v[0]), round(v[1]), v[2], v[3]]
                                     for k2, v in w["cls"].items()}),
                    after=dict(p=round((best[3]["p"] if m["wtf"] == tfs[0]
                                        else 1 - best[3]["p"]) * 100, 1),
                               n1=best[3]["n1"], n2=best[3]["n2"],
                               a1=round(best[3]["a1"]), a2=round(best[3]["a2"]),
                               cls={k2: [round(v[0]), round(v[1]), v[2], v[3]]
                                    for k2, v in best[3]["cls"].items()}),
                    moved=best[1]))
json.dump(out, open("final_steamrolls.json", "w"))
print("\nsteamrolls:")
for o in out:
    print(f"  inst {o['iid']:<6}{o['map'][:24]:<25}{o['kills'][0]}-{o['kills'][1]} kills   "
          f"winner was {o['before']['p']}% -> {o['after']['p']}%   {o['moved']} moved")
