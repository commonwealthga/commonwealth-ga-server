"""Reseed ga_mmr_history using the perf engine, mirroring MmrService.cpp.

Deliberately a line-for-line port of the C++ rather than a tidier rewrite: the
numbers this produces are the acceptance test for the build. If the compiled
engine disagrees with this, one of the two is wrong.
"""
import sqlite3, math, argparse
from collections import defaultdict

# --- ClassProfiles defaults -------------------------------------------------
STATS = ["kills", "assists", "deaths", "damage_dealt", "damage_taken",
         "healing", "defense", "buff_value", "obj_points", "bot_kills",
         "rel_deaths"]
IX = {s: i for i, s in enumerate(STATS)}
DB_COLS = STATS[:10]                       # rel_deaths is derived
NSTAT = len(STATS)

PROFILES = {
    "Assault": dict(profile_id=680, profiles=[
        ("roamer", 1.00, {"damage_dealt": 1.0, "kills": 1.0, "obj_points": 0.4,
                          "assists": 0.3, "deaths": -0.4}),
        ("tank", 1.10, {"damage_taken": 1.0, "obj_points": 1.0, "defense": 0.7,
                        "damage_dealt": 0.4, "kills": 0.3, "deaths": -0.2})],
        rules=[dict(profile="tank", pos=["damage_taken", "obj_points", "defense"],
                    neg=["kills", "damage_dealt"], threshold=0.0, require={})]),
    "Recon": dict(profile_id=681, profiles=[
        ("recon", 1.00, {"kills": 1.0, "damage_dealt": 0.9, "obj_points": 0.4,
                         "assists": 0.3, "deaths": -0.4})], rules=[]),
    "Medic": dict(profile_id=567, profiles=[
        ("healer", 1.00, {"healing": 1.0, "assists": 0.8, "buff_value": 0.4,
                          "obj_points": 0.2, "deaths": -0.2,
                          "damage_taken": -0.4, "rel_deaths": -0.3}),
        ("poison", 0.45, {"kills": 0.8, "damage_dealt": 0.8, "assists": 0.5,
                          "healing": 0.4, "buff_value": 0.2, "obj_points": 0.2,
                          "deaths": -0.3}),
        # Heals heavily AND buffs heavily -- not a trade of one for the other.
        ("buff", 1.00, {"healing": 1.0, "buff_value": 0.8, "assists": 0.6,
                        "obj_points": 0.2, "deaths": -0.2,
                        "damage_taken": -0.4, "rel_deaths": -0.3})],
        # Poison first: a damage medic must never be caught by the buff floors.
        rules=[dict(profile="poison", pos=["kills", "damage_dealt"],
                    neg=["healing"], threshold=0.5, require={}),
               dict(profile="buff", pos=[], neg=[], threshold=-1e9,
                    require={"healing": 0.35, "buff_value": 0.30})]),
    "Robotic": dict(profile_id=679, profiles=[
        ("robo", 1.00, {"damage_dealt": 0.8, "defense": 0.8, "kills": 0.7,
                        "obj_points": 0.4, "healing": 0.4, "assists": 0.3,
                        "deaths": -0.3})], rules=[]),
}
BY_PID = {v["profile_id"]: k for k, v in PROFILES.items()}

TUN = dict(beta=1.0, perf_scale=600.0, elo_divisor=120.0,
           k_base=24.0, k_provisional=48.0, provisional_games=5,
           elo_per_player_gap=110.0, gap_cap=3.0, seed_weight=0.6,
           default_mmr=1000.0, min_seconds=180, min_match_share=0.50,
           min_rated_players=10, z_clamp=3.0, min_baseline_games=20)

SCHEMA = """
DROP TABLE IF EXISTS ga_mmr_history;
CREATE TABLE ga_mmr_history (
  user_id     INTEGER NOT NULL,
  class_name  TEXT    NOT NULL,
  archetype   TEXT    NOT NULL DEFAULT '',
  instance_id INTEGER NOT NULL,
  played_at   INTEGER NOT NULL,
  minutes     REAL    NOT NULL DEFAULT 0,
  match_share REAL    NOT NULL DEFAULT 0,
  perf        REAL    NOT NULL DEFAULT 0,
  expected    REAL    NOT NULL DEFAULT 0,
  actual      REAL    NOT NULL DEFAULT 0,
  mmr_before  REAL    NOT NULL,
  mmr_after   REAL    NOT NULL,
  games_after INTEGER NOT NULL,
  PRIMARY KEY (user_id, class_name, instance_id));
CREATE INDEX IF NOT EXISTS idx_mmr_history_instance ON ga_mmr_history(instance_id);
DELETE FROM ga_mmr_processed;
"""


def zscore(acc, v, clamp, min_games):
    s, ss, n = acc
    if n < min_games:
        return None
    mean = s / n
    var = ss / n - mean * mean
    if var < 0:
        var = 0.0
    sd = math.sqrt(var)
    if sd == 0:
        return None
    return max(-clamp, min(clamp, (v - mean) / sd))


def mean_z(base, names, rate):
    vals = [z for z in (zscore(base[IX[k]], rate[IX[k]],
                               TUN["z_clamp"], TUN["min_baseline_games"])
                        for k in names) if z is not None]
    return (sum(vals) / len(vals)) if vals else None


def load(conn):
    q = lambda s, *a: conn.execute(s, a).fetchall()
    matches = []
    for iid, started, wtf, _ct, mlen in q("""
            SELECT i.id, i.started_at, COALESCE(i.winning_task_force,0),
                   COALESCE(NULLIF(i.end_mission_at,0),NULLIF(i.sealed_at,0),i.started_at) ct,
                   COALESCE((SELECT MAX(e.game_time) FROM ga_match_events e
                             WHERE e.instance_id=i.id),0.0)
            FROM ga_instances i
            WHERE i.outcome IN ('ATTACKERS_WIN','DEFENDERS_WIN','STALEMATE')
              AND EXISTS(SELECT 1 FROM map_game_info m
                         WHERE m.map_name=i.map_name AND m.is_pvp=1)
            ORDER BY ct ASC, i.id ASC"""):
        matches.append(dict(id=iid, started=started, wtf=wtf, length=mlen, players=[]))
    by_id = {m["id"]: m for m in matches}

    team = {}
    for iid, tf, d, t in q("""SELECT instance_id,task_force,SUM(deaths),
                                     SUM(time_played_seconds)
                              FROM ga_match_player_stats GROUP BY 1,2"""):
        team[(iid, tf)] = (d or 0.0, t or 0.0)

    cols = ",".join("s." + c for c in DB_COLS)
    stints = defaultdict(list)
    for row in q(f"""
            SELECT s.instance_id, s.user_id, s.task_force, r.profile_id,
                   s.time_played_seconds, {cols}
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
        iid, uid, tf, pid, t = row[0], row[1], row[2], row[3], row[4]
        if iid not in by_id or pid not in BY_PID:
            continue
        raw = [0.0] * NSTAT
        for k, c in enumerate(DB_COLS):
            raw[IX[c]] = row[5 + k] or 0.0
        stints[(iid, uid)].append(dict(cls=BY_PID[pid], pid=pid, tf=tf,
                                       time=t or 0.0, raw=raw))

    for (iid, uid), grp in stints.items():
        grp.sort(key=lambda x: -x["time"])
        fin = grp[0]
        summed = [0.0] * NSTAT
        ctime = 0.0
        for s in grp:
            if s["cls"] != fin["cls"]:
                continue
            ctime += s["time"]
            for k in range(NSTAT):
                summed[k] += s["raw"][k]
        if ctime < TUN["min_seconds"]:
            continue
        m = by_id[iid]
        share = min(1.0, ctime / m["length"]) if m["length"] > 0 else 1.0
        if share < TUN["min_match_share"]:
            continue
        mins = ctime / 60.0
        rate = [x / mins for x in summed]
        rate[IX["rel_deaths"]] = 1.0
        td, tt = team.get((iid, fin["tf"]), (0.0, 0.0))
        others_min = (tt - ctime) / 60.0
        others_deaths = td - summed[IX["deaths"]]
        if others_min > 1.0 and others_deaths > 0.0:
            rate[IX["rel_deaths"]] = rate[IX["deaths"]] / (others_deaths / others_min)
        m["players"].append(dict(uid=uid, tf=fin["tf"], cls=fin["cls"],
                                 pid=fin["pid"], ctime=ctime, share=share,
                                 rate=rate, perf=0.0, arch=""))
    return matches


def team_size_gap(conn, iid):
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
        if t == "JOIN":
            state[a] = atf
        elif t == "LEAVE":
            state.pop(a, None)
        else:
            state[a] = ttf

    for e in evs:
        if e[1] > settle:
            break
        ap(e)
    for e in evs:
        if e[1] <= settle:
            continue
        if e[1] > teardown:
            break
        dt = e[1] - last
        for v in state.values():
            if v == 1:
                w1 += dt
            elif v == 2:
                w2 += dt
        ap(e)
        last = e[1]
    dur = teardown - settle
    if dur <= 0:
        return (0, 0.0)
    a1, a2 = w1 / dur, w2 / dur
    return (1 if a1 > a2 else 2, abs(a1 - a2))


def seed_rating(rating, games, uid):
    vals = [v for k, v in rating.items()
            if k[0] == uid and games.get(k, 0) >= TUN["provisional_games"]]
    if not vals:
        return TUN["default_mmr"]
    return TUN["default_mmr"] + TUN["seed_weight"] * (
        sum(vals) / len(vals) - TUN["default_mmr"])


def run(path, apply_writes):
    conn = sqlite3.connect(path)
    matches = load(conn)
    if apply_writes:
        conn.executescript(SCHEMA)

    baselines = defaultdict(lambda: [[0.0, 0.0, 0] for _ in range(NSTAT)])
    arch = defaultdict(lambda: [dict(), dict()])   # per-stat z sum, count
    rating, games = {}, defaultdict(int)
    rows = []
    rated = small = one_sided = gap_adj = 0

    for m in matches:
        if len(m["players"]) < TUN["min_rated_players"]:
            small += 1
            continue

        for p in m["players"]:
            d = PROFILES[p["cls"]]
            base = baselines[p["cls"]]
            key = (p["uid"], p["cls"])
            sig = arch[key]

            zs = {}
            for si, sname in enumerate(STATS):
                z = zscore(base[si], p["rate"][si],
                           TUN["z_clamp"], TUN["min_baseline_games"])
                if z is None:
                    continue
                zs[sname] = z
                sig[0][sname] = sig[0].get(sname, 0.0) + z
                sig[1][sname] = sig[1].get(sname, 0) + 1

            def mean_of(name):
                n = sig[1].get(name, 0)
                return (sig[0][name] / n) if n else None

            def group_of(names):
                vals = [v for v in (mean_of(n) for n in names) if v is not None]
                return (sum(vals) / len(vals)) if vals else None

            prof = d["profiles"][0]
            for rule in d["rules"]:
                cand = next((x for x in d["profiles"] if x[0] == rule["profile"]), None)
                if cand is None:
                    continue
                pos = 0.0 if not rule["pos"] else group_of(rule["pos"])
                neg = 0.0 if not rule["neg"] else group_of(rule["neg"])
                if pos is None or neg is None:
                    continue
                if pos - neg <= rule["threshold"]:
                    continue
                ok = True
                for rname, rmin in rule["require"].items():
                    m2 = mean_of(rname)
                    if m2 is None or m2 < rmin:
                        ok = False
                        break
                if not ok:
                    continue
                prof = cand
                break

            name, prem, w = prof
            num = den = 0.0
            for k, wt in w.items():
                if k not in zs:
                    continue
                num += wt * zs[k]
                den += abs(wt)
            p["perf"] = prem * (num / den) if den else 0.0
            p["arch"] = name

        def advance():
            for p in m["players"]:
                b = baselines[p["cls"]]
                for k in range(NSTAT):
                    b[k][0] += p["rate"][k]
                    b[k][1] += p["rate"][k] ** 2
                    b[k][2] += 1

        tfs = sorted({p["tf"] for p in m["players"]})
        if len(tfs) != 2:
            one_sided += 1
            advance()
            continue
        ta, tb = tfs

        big, gap = team_size_gap(conn, m["id"])
        gap = min(gap, TUN["gap_cap"])
        adjust = big in (ta, tb) and gap > 0.0
        if adjust:
            gap_adj += 1

        agg = defaultdict(lambda: [0.0, 0])
        for p in m["players"]:
            k = (p["uid"], p["cls"])
            if k not in rating:
                rating[k] = seed_rating(rating, games, p["uid"])
            agg[p["tf"]][0] += rating[k]
            agg[p["tf"]][1] += 1
        avg = {t: s / c for t, (s, c) in agg.items()}
        adj = dict(avg)
        if adjust:
            small_tf = tb if big == ta else ta
            half = gap * TUN["elo_per_player_gap"] / 2.0
            adj[big] += half
            adj[small_tf] -= half

        new = {}
        for p in m["players"]:
            k = (p["uid"], p["cls"])
            opp = tb if p["tf"] == ta else ta
            before = rating[k]
            # Who won is a TEAM fact: scored team vs team and shared by the
            # side. Scoring an individual against the opposing team treats one
            # player as deciding a 9-a-side match. Per-player feedback comes
            # from the anchored perf term below.
            expected = 1.0 / (1.0 + 10.0 ** ((adj[opp] - adj[p["tf"]]) / TUN["elo_divisor"]))
            actual = 0.5 if m["wtf"] == 0 else (1.0 if p["tf"] == m["wtf"] else 0.0)
            g = games[k]
            K = TUN["k_provisional"] if g < TUN["provisional_games"] else TUN["k_base"]
            # Both halves self-correcting: raw perf would be a constant push
            # with nothing pulling back, and the rating would never settle.
            expected_perf = (before - TUN["default_mmr"]) / TUN["perf_scale"]
            after = before + K * ((actual - expected)
                                  + TUN["beta"] * (p["perf"] - expected_perf))
            new[k] = after
            games[k] = g + 1
            rows.append((p["uid"], p["cls"], p["arch"], m["id"], m["started"],
                         p["ctime"] / 60.0, p["share"], p["perf"], expected,
                         actual, before, after, g + 1))
        rating.update(new)
        rated += 1
        advance()

    if apply_writes:
        conn.executemany(
            "INSERT OR IGNORE INTO ga_mmr_history (user_id,class_name,archetype,"
            "instance_id,played_at,minutes,match_share,perf,expected,actual,"
            "mmr_before,mmr_after,games_after) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)",
            rows)
        conn.executemany(
            "INSERT OR IGNORE INTO ga_mmr_processed (instance_id, played_at) VALUES (?,?)",
            [(m["id"], m["started"]) for m in matches])
        conn.commit()
    conn.close()
    print(f"folded {rated} match(es) ({small} too small, {one_sided} one-sided, "
          f"{gap_adj} gap-adjusted); rows {len(rows)}")
    return rating, games, rows


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("db")
    ap.add_argument("--apply", action="store_true")
    a = ap.parse_args()
    run(a.db, a.apply)
