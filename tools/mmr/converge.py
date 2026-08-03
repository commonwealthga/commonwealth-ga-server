"""Test a convergent update rule against the current drifting one.

Current:   delta = K * ( (actual - expected) + beta * perf )
             -> the perf term is a constant push with nothing pulling back, so
                any player whose beta*perf exceeds (1 - win_rate) rises forever.

Proposed:  delta = K * ( (actual - expected) + beta * (perf - perf_expected) )
           where perf_expected = (rating - default) / perf_scale

             -> a player is only rewarded for OUTPERFORMING their own rating.
                perf_scale is the one new parameter: how many rating points one
                unit of perf is worth at rest. It gives the rating a genuine
                equilibrium instead of a direction of travel.
"""
import sqlite3, math, statistics as st
from collections import defaultdict
import reseed
from reseed import TUN, PROFILES, IX, STATS

DB = "file:E:/ga.db?mode=ro"


def fold(matches, conn, beta, perf_scale, divisor=400.0, warm=40):
    """perf_scale None = the current drifting rule."""
    baselines = defaultdict(lambda: [[0.0, 0.0, 0] for _ in range(len(STATS))])
    arch = defaultdict(lambda: [0.0, 0])
    rating, games = {}, defaultdict(int)
    preds, ll, n, acc = [], 0.0, 0, 0

    for i, m in enumerate(matches):
        if len(m["players"]) < TUN["min_rated_players"]:
            continue
        for p in m["players"]:
            d = PROFILES[p["cls"]]
            base = baselines[p["cls"]]
            prof = d["profiles"][0]
            if len(d["profiles"]) > 1 and d["disc"]:
                pos = reseed.mean_z(base, d["disc"]["pos"], p["rate"])
                neg = reseed.mean_z(base, d["disc"]["neg"], p["rate"])
                k = (p["uid"], p["cls"])
                if pos is not None and neg is not None:
                    arch[k][0] += pos - neg
                    arch[k][1] += 1
                if arch[k][1] and arch[k][0] / arch[k][1] > d["disc"]["threshold"]:
                    prof = d["profiles"][1]
            _nm, prem, w = prof
            num = den = 0.0
            for s, wt in w.items():
                z = reseed.zscore(base[IX[s]], p["rate"][IX[s]],
                                  TUN["z_clamp"], TUN["min_baseline_games"])
                if z is None:
                    continue
                num += wt * z
                den += abs(wt)
            p["perf"] = prem * (num / den) if den else 0.0

        tfs = sorted({p["tf"] for p in m["players"]})
        if len(tfs) == 2:
            ta, tb = tfs
            agg = defaultdict(lambda: [0.0, 0])
            for p in m["players"]:
                k = (p["uid"], p["cls"])
                if k not in rating:
                    rating[k] = reseed.seed_rating(rating, games, p["uid"])
                agg[p["tf"]][0] += rating[k]
                agg[p["tf"]][1] += 1
            avg = {t: s / c for t, (s, c) in agg.items()}
            big, gap = reseed.team_size_gap(conn, m["id"])
            gap = min(gap, TUN["gap_cap"])
            adj = dict(avg)
            if big in (ta, tb) and gap > 0:
                sm = tb if big == ta else ta
                half = gap * TUN["elo_per_player_gap"] / 2.0
                adj[big] += half
                adj[sm] -= half
            if i >= warm and m["wtf"] != 0:
                pr = 1.0 / (1.0 + 10.0 ** ((adj[tb] - adj[ta]) / divisor))
                pr = min(max(pr, 1e-6), 1 - 1e-6)
                y = 1.0 if m["wtf"] == ta else 0.0
                ll += -(y * math.log(pr) + (1 - y) * math.log(1 - pr))
                acc += (pr > .5) == (y == 1)
                n += 1
                preds.append((pr, y))

            new = {}
            for p in m["players"]:
                k = (p["uid"], p["cls"])
                opp = tb if p["tf"] == ta else ta
                b = rating[k]
                e = 1.0 / (1.0 + 10.0 ** ((adj[opp] - b) / divisor))
                a = 0.5 if m["wtf"] == 0 else (1.0 if p["tf"] == m["wtf"] else 0.0)
                g = games[k]
                K = TUN["k_provisional"] if g < TUN["provisional_games"] else TUN["k_base"]
                if perf_scale is None:
                    term = beta * p["perf"]
                else:
                    expected_perf = (b - TUN["default_mmr"]) / perf_scale
                    term = beta * (p["perf"] - expected_perf)
                new[k] = b + K * ((a - e) + term)
                games[k] = g + 1
            rating.update(new)

        for p in m["players"]:
            bl = baselines[p["cls"]]
            for k2 in range(len(STATS)):
                bl[k2][0] += p["rate"][k2]
                bl[k2][1] += p["rate"][k2] ** 2
                bl[k2][2] += 1

    return dict(rating=rating, games=games, ll=ll / n, acc=acc / n, n=n, preds=preds)


def drift_test(res, conn, matches, beta, perf_scale, extra=400):
    """Replay one player's own matches on repeat and see where they end up."""
    hist = defaultdict(list)
    for m in matches:
        if len(m["players"]) < TUN["min_rated_players"]:
            continue
        for p in m["players"]:
            hist[(p["uid"], p["cls"])].append(p["perf"])
    out = {}
    for key, perfs in hist.items():
        if len(perfs) < 60:
            continue
        r, g = 1000.0, 0
        track = {}
        for i in range(extra):
            pf = perfs[i % len(perfs)]
            # neutral opponent, empirical win rate for this key
            e = 1.0 / (1.0 + 10.0 ** ((1050.0 - r) / 400.0))
            a = 0.60
            K = TUN["k_provisional"] if g < TUN["provisional_games"] else TUN["k_base"]
            term = beta * pf if perf_scale is None else \
                beta * (pf - (r - 1000.0) / perf_scale)
            r += K * ((a - e) + term)
            g += 1
            if g in (100, 200, 400):
                track[g] = round(r)
        out[key] = track
    return out


if __name__ == "__main__":
    conn = sqlite3.connect(DB, uri=True)
    matches = reseed.load(conn)
    print("rule                                 logloss    acc   spread(>=20g)   drift 100->400")
    for label, beta, scale in (("current (drifting)", 1.0, None),
                               ("anchored, scale 400", 1.0, 400.0),
                               ("anchored, scale 600", 1.0, 600.0),
                               ("anchored, scale 800", 1.0, 800.0),
                               ("anchored, scale 1200", 1.0, 1200.0)):
        r = fold(matches, conn, beta, scale)
        vals = sorted(v for k, v in r["rating"].items() if r["games"][k] >= 20)
        d = drift_test(r, conn, matches, beta, scale)
        sample = next(iter(d.values())) if d else {}
        dr = f"{sample.get(100,'')}->{sample.get(400,'')}" if sample else "n/a"
        print(f"{label:<34}{r['ll']:>9.4f}{r['acc']*100:>6.0f}%"
              f"   {min(vals):.0f}-{max(vals):.0f} (sd {st.pstdev(vals):.0f})   {dr}")
