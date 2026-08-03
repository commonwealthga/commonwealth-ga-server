"""How balanced were team sizes, for the majority of each match?

Walks the JOIN / LEAVE / TEAM_CHANGE log and accumulates wall time against each
(team1, team2) headcount configuration, so a match is classified by what was
actually true for most of it rather than by who happened to be present at the
start or the end.

TEAM_CHANGE destination comes from `detail` -- target_task_force is NULL on
every row, and reading it drops the mover from both counts for the rest of the
match.
"""
import sqlite3, statistics as st
from collections import defaultdict

DB = "file:E:/server-redacted.db?mode=ro"
MIN_TOTAL = 10


def dominant_split(conn, iid):
    """Returns (bigger, smaller, share_of_match, mean_total) or None."""
    evs = conn.execute("""SELECT event_type,game_time,COALESCE(actor_user_id,0),
            COALESCE(actor_task_force,0),COALESCE(target_task_force,detail,0)
            FROM ga_match_events WHERE instance_id=?
              AND event_type IN('JOIN','LEAVE','TEAM_CHANGE')
              AND game_time IS NOT NULL ORDER BY ts""", (iid,)).fetchall()
    if len(evs) < 2:
        return None
    # Settle past the join ramp and stop before the teardown, same window the
    # rating engine uses for its headcount correction.
    i = 0
    while i < len(evs) and evs[i][0] == "JOIN":
        i += 1
    settle = evs[i - 1 if i > 0 else 0][1]
    j = len(evs) - 1
    while j >= 0 and evs[j][0] == "LEAVE":
        j -= 1
    teardown = evs[j + 1][1] if j + 1 < len(evs) else evs[-1][1]
    if teardown <= settle:
        return None

    state, last = {}, settle
    held = defaultdict(float)
    weighted_total, total_time = 0.0, 0.0

    def apply(e):
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
        apply(e)
    for e in evs:
        if e[1] <= settle:
            continue
        if e[1] > teardown:
            break
        dt = e[1] - last
        if dt > 0:
            n1 = sum(1 for v in state.values() if v == 1)
            n2 = sum(1 for v in state.values() if v == 2)
            held[(max(n1, n2), min(n1, n2))] += dt
            weighted_total += (n1 + n2) * dt
            total_time += dt
        apply(e)
        last = e[1]
    if total_time <= 0 or not held:
        return None
    (big, small), t = max(held.items(), key=lambda kv: kv[1])
    return big, small, t / total_time, weighted_total / total_time


if __name__ == "__main__":
    conn = sqlite3.connect(DB, uri=True)
    ids = [r[0] for r in conn.execute("""
        SELECT i.id FROM ga_instances i
        WHERE i.outcome IN ('ATTACKERS_WIN','DEFENDERS_WIN','STALEMATE')
          AND EXISTS(SELECT 1 FROM map_game_info m
                     WHERE m.map_name=i.map_name AND m.is_pvp=1)""")]
    splits, shares, skipped = defaultdict(int), [], 0
    for iid in ids:
        d = dominant_split(conn, iid)
        if d is None:
            skipped += 1
            continue
        big, small, share, mean_total = d
        if big + small < MIN_TOTAL:
            continue
        splits[(big, small)] += 1
        shares.append(share)

    total = sum(splits.values())
    print(f"{total} matches with {MIN_TOTAL}+ players, classified by the split that held "
          f"for the largest share of the match")
    print(f"({skipped} of {len(ids)} had no usable event log)\n")

    print(f"{'split':>8}{'matches':>9}{'share':>8}")
    for (big, small), n in sorted(splits.items(), key=lambda kv: (-kv[1], kv[0])):
        print(f"{f'{big}v{small}':>8}{n:>9}{n/total*100:>7.0f}%")

    print(f"\n{'gap':>8}{'matches':>9}{'share':>8}")
    bygap = defaultdict(int)
    for (big, small), n in splits.items():
        bygap[big - small] += n
    cum = 0
    for gap in sorted(bygap):
        cum += bygap[gap]
        label = "even" if gap == 0 else f"+{gap}"
        print(f"{label:>8}{bygap[gap]:>9}{bygap[gap]/total*100:>7.0f}%"
              f"   (cumulative {cum/total*100:.0f}%)")

    print(f"\nthe dominant split held for a median {st.median(shares)*100:.0f}% of the match; "
          f"{sum(1 for s in shares if s >= 0.5)/len(shares)*100:.0f}% of matches "
          f"spent over half their time in one configuration")
