"""Leaderboard from the last two Sundays only -- a form table, not a career one.

Everyone restarts at 1000 and only those matches are folded, so the numbers say
"how this fortnight went", not "how good you are". Class baselines are kept from
the full dataset on purpose: what counts as an average medic is a property of
the class, not of a 37-match window, and re-deriving it from so little would
make every z-score meaningless.
"""
import sqlite3, math, json, datetime, statistics as st
from collections import defaultdict
import reseed
from reseed import TUN, PROFILES, STATS, IX

DB = "file:E:/server-redacted.db?mode=ro"
conn = sqlite3.connect(DB, uri=True)
col = next(x[1] for x in conn.execute("PRAGMA table_info(ga_users)")
           if x[1] in ("username", "name", "display_name"))
NM = {u: n for u, n in conn.execute(f"SELECT id,{col} FROM ga_users")}

allm = reseed.load(conn)
rated = [m for m in allm if len(m["players"]) >= TUN["min_rated_players"]]
sundays = sorted({datetime.datetime.fromtimestamp(m["started"], datetime.timezone.utc).date()
                  for m in rated
                  if datetime.datetime.fromtimestamp(m["started"], datetime.timezone.utc)
                  .weekday() == 6})[-2:]
window = [m for m in rated
          if datetime.datetime.fromtimestamp(m["started"], datetime.timezone.utc).date()
          in sundays]
print(f"window: {sundays[0]} and {sundays[1]}  ->  {len(window)} matches")

# Class baselines from the FULL dataset (see module docstring).
base = defaultdict(lambda: [[0.0, 0.0, 0] for _ in range(len(STATS))])
for m in rated:
    for p in m["players"]:
        b = base[p["cls"]]
        for k in range(len(STATS)):
            b[k][0] += p["rate"][k]; b[k][1] += p["rate"][k] ** 2; b[k][2] += 1

arch = defaultdict(lambda: [dict(), dict()])
rating, games, wins, exl, pfl = {}, defaultdict(int), defaultdict(lambda: [0, 0]), \
    defaultdict(list), defaultdict(list)
arch_last = {}

for m in window:
    for p in m["players"]:
        d = PROFILES[p["cls"]]; b = base[p["cls"]]
        key = (p["uid"], p["cls"]); sig = arch[key]
        zs = {}
        for si, sname in enumerate(STATS):
            z = reseed.zscore(b[si], p["rate"][si], TUN["z_clamp"], TUN["min_baseline_games"])
            if z is None:
                continue
            zs[sname] = z
            sig[0][sname] = sig[0].get(sname, 0.0) + z
            sig[1][sname] = sig[1].get(sname, 0) + 1
        mean_of = lambda n: (sig[0][n] / sig[1][n]) if sig[1].get(n) else None
        def group_of(names):
            v = [x for x in (mean_of(n) for n in names) if x is not None]
            return (sum(v) / len(v)) if v else None
        prof = d["profiles"][0]
        for rule in d["rules"]:
            cand = next((x for x in d["profiles"] if x[0] == rule["profile"]), None)
            if cand is None: continue
            pos = 0.0 if not rule["pos"] else group_of(rule["pos"])
            neg = 0.0 if not rule["neg"] else group_of(rule["neg"])
            if pos is None or neg is None: continue
            if pos - neg <= rule["threshold"]: continue
            ok = all((mean_of(rn) is not None and mean_of(rn) >= rv)
                     for rn, rv in rule["require"].items())
            if not ok: continue
            prof = cand; break
        nme, prem, w = prof
        num = den = 0.0
        for k2, wt in w.items():
            if k2 not in zs: continue
            num += wt * zs[k2]; den += abs(wt)
        p["perf"] = prem * (num / den) if den else 0.0
        p["arch"] = nme

    tfs = sorted({p["tf"] for p in m["players"]})
    if len(tfs) != 2:
        continue
    ta, tb = tfs
    agg = defaultdict(lambda: [0.0, 0])
    for p in m["players"]:
        k = (p["uid"], p["cls"])
        if k not in rating:
            rating[k] = reseed.seed_rating(rating, games, p["uid"])
        agg[p["tf"]][0] += rating[k]; agg[p["tf"]][1] += 1
    avg = {t: s / c for t, (s, c) in agg.items()}
    big, gap = reseed.team_size_gap(conn, m["id"]); gap = min(gap, TUN["gap_cap"])
    adj = dict(avg)
    if big in (ta, tb) and gap > 0:
        sm = tb if big == ta else ta
        half = gap * TUN["elo_per_player_gap"] / 2.0
        adj[big] += half; adj[sm] -= half
    new = {}
    for p in m["players"]:
        k = (p["uid"], p["cls"]); opp = tb if p["tf"] == ta else ta
        b0 = rating[k]
        e = 1.0 / (1.0 + 10.0 ** ((adj[opp] - adj[p["tf"]]) / TUN["elo_divisor"]))
        a = 0.5 if m["wtf"] == 0 else (1.0 if p["tf"] == m["wtf"] else 0.0)
        g = games[k]
        K = TUN["k_provisional"] if g < TUN["provisional_games"] else TUN["k_base"]
        new[k] = b0 + K * ((a - e) + TUN["beta"] *
                           (p["perf"] - (b0 - TUN["default_mmr"]) / TUN["perf_scale"]))
        games[k] = g + 1; wins[k][1] += 1; wins[k][0] += 1 if a == 1.0 else 0
        exl[k].append(e); pfl[k].append(p["perf"]); arch_last[k] = p["arch"]
    rating.update(new)

# career ratings, for the comparison column
career = {}
for u, cl, v in conn.execute(
        "SELECT user_id,class_name,mmr_after FROM ga_mmr_history ORDER BY instance_id"):
    career[(u, cl)] = v

MING = 5
out = {}
for cls in ("Assault", "Recon", "Medic", "Robotic"):
    rows = []
    for k, v in rating.items():
        if k[1] != cls or games[k] < MING:
            continue
        rows.append(dict(name=(NM.get(k[0]) or f"u{k[0]}")[:18], mmr=round(v),
                         games=games[k], arch=arch_last.get(k, ""),
                         perf=round(st.mean(pfl[k]), 2),
                         win=round(wins[k][0] / wins[k][1] * 100),
                         career=(round(career[k]) if k in career else None)))
    rows.sort(key=lambda r: -r["mmr"])
    out[cls] = rows
json.dump(dict(rows=out, days=[str(d) for d in sundays], matches=len(window)),
          open("recent.json", "w"))

tot = sum(len(v) for v in out.values())
allv = [r["mmr"] for v in out.values() for r in v]
print(f"{tot} rated at {MING}+ games in the window, spread {min(allv)}-{max(allv)}\n")
for cls in ("Assault", "Recon", "Medic", "Robotic"):
    print(f"--- {cls} ({len(out[cls])}) ---")
    for i, r in enumerate(out[cls], 1):
        d = (r["mmr"] - r["career"]) if r["career"] else 0
        print(f"  {i:>2}. {r['name']:<18}{r['mmr']:>6}{r['games']:>4}g "
              f"{r['win']:>4}% perf {r['perf']:+.2f}  {r['arch']:<7}"
              f"  career {r['career'] or '-':>5} ({d:+d})")
    print()
