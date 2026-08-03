"""Forward-project the win/loss engine 10 games, old divisor vs recalibrated.

Pure win/loss -- no performance term. Each player is projected using their own
empirical win rate and the average opposition they have actually faced, so the
only thing changing between the two columns is the divisor.

A pure Elo rating settles where its expectation matches its win rate, i.e. at
  opponent + divisor * log10(wr / (1-wr))
so a wider divisor spreads the same win rates over a wider band. That is the
whole effect being measured here.
"""
import sqlite3, math, statistics as st, json
from collections import defaultdict
import wl_divisor as W

conn = sqlite3.connect(W.DB, uri=True)
matches = W.load(conn)
col = next(x[1] for x in conn.execute("PRAGMA table_info(ga_users)")
           if x[1] in ("username", "name", "display_name"))
NM = {u: n for u, n in conn.execute(f"SELECT id,{col} FROM ga_users")}


def fold(divisor):
    """Replay the engine; also record each player's mean opposition."""
    rating, games = {}, defaultdict(int)
    wins = defaultdict(lambda: [0, 0])
    opp = defaultdict(list)
    for m in matches:
        tfs = sorted({p["tf"] for p in m["players"]})
        if len(tfs) != 2:
            continue
        ta, tb = tfs
        agg = defaultdict(lambda: [0.0, 0])
        for p in m["players"]:
            k = (p["uid"], p["cls"])
            rating.setdefault(k, W.DEFAULT)
            agg[p["tf"]][0] += rating[k]
            agg[p["tf"]][1] += 1
        avg = {t: s / c for t, (s, c) in agg.items()}
        big, gap = W.team_gap(conn, m["id"])
        gap = min(gap, W.GAP_CAP)
        adj = dict(avg)
        if big in (ta, tb) and gap > W.DEADZONE:
            sm = tb if big == ta else ta
            half = gap * W.GAP_ELO / 2.0
            adj[big] += half
            adj[sm] -= half
        new = {}
        for p in m["players"]:
            k = (p["uid"], p["cls"])
            o = tb if p["tf"] == ta else ta
            b = rating[k]
            e = 1.0 / (1.0 + 10.0 ** ((adj[o] - b) / divisor))
            a = 0.5 if m["wtf"] == 0 else (1.0 if p["tf"] == m["wtf"] else 0.0)
            new[k] = b + W.K * (a - e)
            games[k] += 1
            wins[k][1] += 1
            wins[k][0] += 1 if a == 1.0 else 0
            opp[k].append(adj[o])
        rating.update(new)
    return rating, games, wins, opp


def project(r, wr, oppr, divisor, n):
    """n more matches at the same win rate against the same standard."""
    for _ in range(n):
        e = 1.0 / (1.0 + 10.0 ** ((oppr - r) / divisor))
        r += W.K * (wr - e)
    return r


def equilibrium(wr, oppr, divisor):
    """Where a pure Elo rating comes to rest."""
    wr = min(max(wr, 0.001), 0.999)
    return oppr + divisor * math.log10(wr / (1.0 - wr))


if __name__ == "__main__":
    OLD, NEW, AHEAD = 400.0, 900.0, 10
    r_old, g, wins, opp_old = fold(OLD)
    r_new, _, _, opp_new = fold(NEW)

    MIN = {"Assault": 20, "Recon": 20, "Medic": 20, "Robotic": 10}
    out = {}
    for cls in ("Assault", "Recon", "Medic", "Robotic"):
        rows = []
        for k in r_old:
            if k[1] != cls or g[k] < MIN[cls]:
                continue
            wr = wins[k][0] / wins[k][1]
            oo, on = st.mean(opp_old[k]), st.mean(opp_new[k])
            rows.append(dict(
                name=(NM.get(k[0]) or f"u{k[0]}")[:17], games=g[k], win=round(wr * 100),
                old=round(r_old[k]), old10=round(project(r_old[k], wr, oo, OLD, AHEAD)),
                old_eq=round(equilibrium(wr, oo, OLD)),
                new=round(r_new[k]), new10=round(project(r_new[k], wr, on, NEW, AHEAD)),
                new_eq=round(equilibrium(wr, on, NEW))))
        rows.sort(key=lambda x: -x["new"])
        out[cls] = rows
    json.dump(out, open("wl_project.json", "w"))

    allr = [r for v in out.values() for r in v]
    for lab, a, b in (("now", "old", "new"), ("+10 games", "old10", "new10"),
                      ("at rest", "old_eq", "new_eq")):
        va = [r[a] for r in allr]
        vb = [r[b] for r in allr]
        print(f"{lab:<12} divisor 400: {min(va):>5}-{max(va):<5} sd {st.pstdev(va):>3.0f}"
              f"    divisor 900: {min(vb):>5}-{max(vb):<5} sd {st.pstdev(vb):>3.0f}")
    print()
    for cls in ("Assault", "Recon", "Medic", "Robotic"):
        print(f"--- {cls} ---")
        print(f"{'player':<18}{'g':>4}{'win':>5}   "
              f"{'DIV 400: now':>13}{'+10':>7}{'rest':>7}   "
              f"{'DIV 900: now':>13}{'+10':>7}{'rest':>7}")
        for r in out[cls]:
            print(f"{r['name']:<18}{r['games']:>4}{r['win']:>4}%   "
                  f"{r['old']:>13}{r['old10']:>7}{r['old_eq']:>7}   "
                  f"{r['new']:>13}{r['new10']:>7}{r['new_eq']:>7}")
        print()
