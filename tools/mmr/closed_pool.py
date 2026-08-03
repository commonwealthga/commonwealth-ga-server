"""Does a small, closed playerbase limit what win/loss can tell us?

Everyone plays with and against everyone else, repeatedly. Two consequences
worth measuring:

  1. Teams are drawn from the same pool, so your win rate is mostly decided by
     the eight other people on your side, not by you.
  2. Ratings are zero-sum inside the pool -- the average is pinned near the
     default, so any spread one player gains, others must lose.

The test: keep every real roster and every real result, but shuffle WHICH
players were on the winning side. That produces the win-rate spread you would
see from pure chance on this exact schedule. If the real spread is not much
wider, win rate is not carrying the skill signal a wider divisor would amplify.
"""
import sqlite3, math, random, statistics as st
from collections import defaultdict
import wl_divisor as W

random.seed(20260803)
conn = sqlite3.connect(W.DB, uri=True)
matches = [m for m in W.load(conn) if len(m["players"]) >= 10]
col = next(x[1] for x in conn.execute("PRAGMA table_info(ga_users)")
           if x[1] in ("username", "name", "display_name"))
NM = {u: n for u, n in conn.execute(f"SELECT id,{col} FROM ga_users")}

# ---- how closed is the pool? ----------------------------------------------
appear = defaultdict(int)
pair_same, pair_opp = defaultdict(int), defaultdict(int)
for m in matches:
    ps = m["players"]
    for p in ps:
        appear[p["uid"]] += 1
    for i in range(len(ps)):
        for j in range(i + 1, len(ps)):
            a, b = sorted((ps[i]["uid"], ps[j]["uid"]))
            if ps[i]["tf"] == ps[j]["tf"]:
                pair_same[(a, b)] += 1
            else:
                pair_opp[(a, b)] += 1

regulars = [u for u, n in appear.items() if n >= 20]
print(f"{len(matches)} matches, {len(appear)} distinct players, "
      f"{len(regulars)} with 20+ appearances")
tot = sum(appear.values())
top = sorted(appear.values(), reverse=True)
print(f"the {len(regulars)} regulars account for "
      f"{sum(n for n in appear.values() if n >= 20)/tot*100:.0f}% of all appearances")

# for a regular, what share of matches is any other regular also in?
co = []
for a in regulars:
    for b in regulars:
        if a >= b:
            continue
        n = pair_same[(a, b)] + pair_opp[(a, b)]
        if n:
            co.append(n / min(appear[a], appear[b]))
print(f"two regulars share a match {st.median(co)*100:.0f}% of the time (median pair)")
same_share = [pair_same[k] / (pair_same[k] + pair_opp[k])
              for k in set(pair_same) | set(pair_opp)
              if pair_same[k] + pair_opp[k] >= 10]
print(f"when two players are both in a match they are TEAMMATES "
      f"{st.median(same_share)*100:.0f}% of the time (median pair)\n")

# ---- observed vs chance win-rate spread ------------------------------------
obs = defaultdict(lambda: [0, 0])
for m in matches:
    if m["wtf"] == 0:
        continue
    for p in m["players"]:
        obs[p["uid"]][1] += 1
        obs[p["uid"]][0] += 1 if p["tf"] == m["wtf"] else 0
MING = 20
real = {u: w / n for u, (w, n) in obs.items() if n >= MING}
print(f"{len(real)} players with {MING}+ decided matches")
print(f"OBSERVED win rate: {min(real.values())*100:.0f}%-{max(real.values())*100:.0f}%, "
      f"sd {st.pstdev(list(real.values()))*100:.1f}pp")

TRIALS = 400
sds, spans = [], []
for _ in range(TRIALS):
    sim = defaultdict(lambda: [0, 0])
    for m in matches:
        if m["wtf"] == 0:
            continue
        ps = m["players"]
        n_win = sum(1 for p in ps if p["tf"] == m["wtf"])
        ids = [p["uid"] for p in ps]
        random.shuffle(ids)
        winners = set(ids[:n_win])
        for u in ids:
            sim[u][1] += 1
            sim[u][0] += 1 if u in winners else 0
    vals = [w / n for u, (w, n) in sim.items() if n >= MING]
    sds.append(st.pstdev(vals))
    spans.append(max(vals) - min(vals))
print(f"CHANCE  win rate: sd {st.mean(sds)*100:.1f}pp "
      f"(95% of trials {sorted(sds)[10]*100:.1f}-{sorted(sds)[-10]*100:.1f}pp)")
real_sd = st.pstdev(list(real.values()))
print(f"\nreal sd {real_sd*100:.1f}pp vs chance {st.mean(sds)*100:.1f}pp "
      f"-> {real_sd/st.mean(sds):.2f}x")
above = sum(1 for s in sds if s >= real_sd)
print(f"{above}/{TRIALS} random shuffles produced a spread this wide or wider")

# how much of the real spread is signal?
signal_var = max(0.0, real_sd**2 - st.mean(sds)**2)
print(f"variance attributable to skill: {signal_var/real_sd**2*100:.0f}% "
      f"(=> genuine win-rate sd about {math.sqrt(signal_var)*100:.1f}pp)")
