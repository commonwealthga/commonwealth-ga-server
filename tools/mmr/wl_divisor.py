"""Would changing the Elo divisor improve the CURRENT (live) wl engine?

Faithful replay of the shipped win/loss engine -- K=32, its own participation
rule (>=180s on the finishing class, no match-share or roster minimum), the
220-per-player headcount correction with a 0.25 deadzone -- sweeping only the
divisor. Prediction is scored out of sample: each match is predicted from
ratings built solely from earlier matches.
"""
import sqlite3, math, statistics as st
from collections import defaultdict

DB = "file:E:/server-redacted.db?mode=ro"
CLS = {680: "Assault", 567: "Medic", 681: "Recon", 679: "Robotic"}
K, GAP_ELO, DEADZONE, GAP_CAP, DEFAULT = 32.0, 220.0, 0.25, 3.0, 1000.0


def load(conn):
    q = lambda s, *a: conn.execute(s, a).fetchall()
    matches, by_id = [], {}
    for iid, started, wtf in q("""
            SELECT i.id, i.started_at, COALESCE(i.winning_task_force,0)
            FROM ga_instances i
            WHERE i.outcome IN ('ATTACKERS_WIN','DEFENDERS_WIN','STALEMATE')
              AND EXISTS(SELECT 1 FROM map_game_info m
                         WHERE m.map_name=i.map_name AND m.is_pvp=1)
            ORDER BY COALESCE(NULLIF(i.end_mission_at,0),NULLIF(i.sealed_at,0),
                              i.started_at) ASC, i.id ASC"""):
        m = dict(id=iid, started=started, wtf=wtf, players=[])
        matches.append(m); by_id[iid] = m
    stints = defaultdict(list)
    for iid, uid, tf, pid, t in q("""
            SELECT s.instance_id,s.user_id,s.task_force,r.profile_id,
                   s.time_played_seconds
            FROM ga_match_player_stats s
            JOIN ga_instances i ON i.id=s.instance_id
            JOIN (SELECT instance_id,character_id,MIN(profile_id) profile_id
                  FROM ga_instance_players WHERE profile_id IN(680,567,681,679)
                  GROUP BY instance_id,character_id) r
              ON r.instance_id=s.instance_id AND r.character_id=s.character_id
            WHERE i.outcome IN ('ATTACKERS_WIN','DEFENDERS_WIN','STALEMATE')
              AND (s.kills+s.assists+s.deaths+s.damage_dealt+s.healing
                   +s.obj_points+s.bot_kills) > 0
              AND EXISTS(SELECT 1 FROM map_game_info m
                         WHERE m.map_name=i.map_name AND m.is_pvp=1)"""):
        if iid not in by_id or pid not in CLS:
            continue
        stints[(iid, uid)].append((t or 0.0, CLS[pid], tf))
    for (iid, uid), grp in stints.items():
        grp.sort(key=lambda x: -x[0])
        cls, tf = grp[0][1], grp[0][2]
        total = sum(t for t, c, _ in grp if c == cls)
        if total < 180:
            continue
        by_id[iid]["players"].append(dict(uid=uid, cls=cls, tf=tf))
    return matches


def team_gap(conn, iid):
    evs = conn.execute("""SELECT event_type,game_time,COALESCE(actor_user_id,0),
            COALESCE(actor_task_force,0),COALESCE(target_task_force,detail,0)
            FROM ga_match_events WHERE instance_id=?
              AND event_type IN('JOIN','LEAVE','TEAM_CHANGE')
              AND game_time IS NOT NULL ORDER BY ts""", (iid,)).fetchall()
    if len(evs) < 2:
        return (0, 0.0)
    i = 0
    while i < len(evs) and evs[i][0] == "JOIN":
        i += 1
    settle = evs[i - 1 if i > 0 else 0][1]
    j = len(evs) - 1
    while j >= 0 and evs[j][0] == "LEAVE":
        j -= 1
    teardown = evs[j + 1][1] if j + 1 < len(evs) else evs[-1][1]
    if teardown <= settle:
        return (0, 0.0)
    state, w1, w2, last = {}, 0.0, 0.0, settle

    def ap(e):
        t, _, a, atf, ttf = e
        if t == "JOIN":   state[a] = atf
        elif t == "LEAVE": state.pop(a, None)
        else:              state[a] = ttf
    for e in evs:
        if e[1] > settle: break
        ap(e)
    for e in evs:
        if e[1] <= settle: continue
        if e[1] > teardown: break
        dt = e[1] - last
        for v in state.values():
            if v == 1: w1 += dt
            elif v == 2: w2 += dt
        ap(e); last = e[1]
    dur = teardown - settle
    if dur <= 0:
        return (0, 0.0)
    a1, a2 = w1 / dur, w2 / dur
    return (1 if a1 > a2 else 2, abs(a1 - a2))


def run(matches, conn, divisor, warm=40, min_players=10):
    rating, games = {}, defaultdict(int)
    ll = n = acc = 0
    preds = []
    for i, m in enumerate(matches):
        tfs = sorted({p["tf"] for p in m["players"]})
        if len(tfs) != 2:
            continue
        ta, tb = tfs
        agg = defaultdict(lambda: [0.0, 0])
        for p in m["players"]:
            k = (p["uid"], p["cls"])
            rating.setdefault(k, DEFAULT)
            agg[p["tf"]][0] += rating[k]; agg[p["tf"]][1] += 1
        avg = {t: s / c for t, (s, c) in agg.items()}
        big, gap = team_gap(conn, m["id"])
        gap = min(gap, GAP_CAP)
        adj = dict(avg)
        if big in (ta, tb) and gap > DEADZONE:
            sm = tb if big == ta else ta
            half = gap * GAP_ELO / 2.0
            adj[big] += half; adj[sm] -= half
        # Scored only on matches the new engine would also count, so the two
        # are compared on the same games.
        if i >= warm and m["wtf"] != 0 and len(m["players"]) >= min_players:
            pr = min(max(1.0 / (1.0 + 10.0 ** ((adj[tb] - adj[ta]) / divisor)), 1e-6), 1 - 1e-6)
            y = 1.0 if m["wtf"] == ta else 0.0
            ll += -(y * math.log(pr) + (1 - y) * math.log(1 - pr))
            acc += (pr > .5) == (y == 1); n += 1
            preds.append((pr, y))
        new = {}
        for p in m["players"]:
            k = (p["uid"], p["cls"]); opp = tb if p["tf"] == ta else ta
            b = rating[k]
            e = 1.0 / (1.0 + 10.0 ** ((adj[opp] - b) / divisor))
            a = 0.5 if m["wtf"] == 0 else (1.0 if p["tf"] == m["wtf"] else 0.0)
            new[k] = b + K * (a - e); games[k] += 1
        rating.update(new)
    return rating, games, ll / n, acc / n, n, preds


if __name__ == "__main__":
    conn = sqlite3.connect(DB, uri=True)
    matches = load(conn)
    print(f"{sum(1 for m in matches if len(m['players'])>=10)} matches with >=10 rated players\n")
    print(f"{'divisor':>8}{'logloss':>10}{'acc':>6}   spread(>=20g)      calibration by band")
    for div in (400, 300, 250, 200, 150, 120, 100):
        r, g, ll, acc, n, preds = run(matches, conn, div)
        v = sorted(x for k, x in r.items() if g[k] >= 20)
        pts = [(p, y) for p, y in preds] + [(1 - p, 1 - y) for p, y in preds]
        bands = []
        for lo, hi in ((.55, .65), (.65, .75), (.75, 1.01)):
            sel = [(p, y) for p, y in pts if lo <= p < hi]
            bands.append(f"{sum(p for p,_ in sel)/len(sel)*100:.0f}->{sum(y for _,y in sel)/len(sel)*100:.0f}"
                         if len(sel) >= 8 else "--")
        mark = "  <- live" if div == 400 else ""
        print(f"{div:>8}{ll:>10.4f}{acc*100:>5.0f}%   {min(v):.0f}-{max(v):.0f} (sd {st.pstdev(v):.0f})"
              f"   {'  '.join(bands)}{mark}")
    print(f"\n(scored on {n} matches)")
