#!/usr/bin/env python3
"""Regenerate dbbase/base.db - the schema+content baseline every database starts from.

Why this exists
---------------
Migrations used to live in two places, each bumping the same `version_info`
counter: the game DLL's Database::Init (11,753 lines, target 173) and the
control server's (3,075 lines, floor-write 19). Two writers, one counter,
different histories per file. A node database created fresh sits at the control
server's version, so the DLL's entire ladder replayed against it on every
instance start -- including `INSERT OR REPLACE` seeds with partial column lists,
which reset every column they do not mention. That is how a curated
`map_game_info.is_pvp=1` became 0 on a node, which made a PvP map classify itself
as PvE, which made `TgDeploy_Beacon.PickUpDeployable` refuse beacon pickup.

The replacement is one baseline database plus forward-only .sql migrations
applied by the control server alone. This script produces the baseline from a
live database, so the baseline is always "the state after every migration that
ever ran".

The copy-forward approach is deliberate: schema is recreated in a new file and
rows are copied table by table, rather than deleting rows from a copy. Damage
confined to one table therefore cannot poison the result -- which matters,
because the source this was first built from has malformed pages inside
`ga_match_events` and its index.

Usage:
    python3 scripts/make_dbbase.py                 # server.db -> dbbase/base.db
    python3 scripts/make_dbbase.py --source X --out Y --version N
"""

import argparse
import os
import sqlite3
import sys

# Per-player and per-match state. The schema for these stays in the baseline; the
# rows do not. This is the difference between "a backup" and "a fresh install".
PURGE = [
    # accounts & identity
    "ga_users",
    "ga_user_components",
    "ga_user_preferences",
    "ga_user_roles",
    "ga_user_dlc",
    "ga_user_sessions",
    "ga_user_bans",
    "ga_ip_checks",
    "ga_ip_bans",
    "ga_friends",
    # characters
    "ga_characters",
    "ga_character_devices",
    "ga_character_skills",
    "ga_character_quests",
    "ga_character_quest_progress",
    # in-match presence
    "ga_players",
    "ga_players_inventory",
    # social organisations
    "ga_agencies",
    "ga_agency_members",
    "ga_agency_ranks",
    "ga_alliances",
    "ga_alliance_members",
    # rating history
    "ga_mmr_history",
    "ga_wl_mmr_history",
    "ga_mmr_processed",
    "ga_wl_mmr_processed",
    # instances & matches
    "ga_instances",
    "ga_instance_players",
    "ga_match_events",
    "ga_match_player_stats",
    "ga_match_device_stats",
    "ga_matchmaking_fairness_events",
    # control-server bookkeeping superseded by version-tracked migrations
    "cs_migration_markers",
]

# Content and server config. Asserted non-empty: if one of these comes out empty
# the baseline is broken and every fresh install would start with a blank world.
KEEP_NONEMPTY = [
    "asm_data_set_properties",
    "asm_data_set_devices",
    "asm_data_set_effect_groups",
    "asm_data_set_items",
    "asm_data_set_valid_values",
    "map_game_info",
    "map_object_config",
    "ga_queues",
    "ga_map_pools",
    "ga_map_pool_entries",
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", default="server.db")
    ap.add_argument("--out", default="dbbase/base.db")
    ap.add_argument("--version", type=int, default=None,
                    help="version_info to stamp (default: the source's current version)")
    args = ap.parse_args()

    if not os.path.exists(args.source):
        sys.exit(f"source database not found: {args.source}")

    tmp = args.out + ".tmp"
    if os.path.exists(tmp):
        os.remove(tmp)
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)

    db = sqlite3.connect(tmp)
    # Generated artifact, not an operated database: no journal, no fsyncs.
    db.execute("PRAGMA journal_mode=OFF")
    db.execute("PRAGMA synchronous=OFF")
    db.execute("PRAGMA cache_size=-131072")
    db.execute("ATTACH DATABASE ? AS s", (os.path.abspath(args.source),))

    version = args.version
    if version is None:
        row = db.execute("SELECT max(version) FROM s.version_info").fetchone()
        version = row[0] if row and row[0] is not None else 0

    objs = db.execute(
        "SELECT type, name, sql FROM s.sqlite_master "
        "WHERE sql IS NOT NULL AND name NOT LIKE 'sqlite_%' "
        "ORDER BY CASE type WHEN 'table' THEN 0 WHEN 'index' THEN 1 ELSE 2 END, name"
    ).fetchall()
    tables = [r[1] for r in objs if r[0] == "table"]
    purge = set(PURGE)

    def readable(t):
        try:
            db.execute(f'SELECT count(*) FROM s."{t}" NOT INDEXED').fetchone()
            return True
        except sqlite3.DatabaseError:
            return False

    damaged = [t for t in tables if not readable(t)]
    fatal = [t for t in damaged if t not in purge]
    if fatal:
        db.close()
        os.remove(tmp)
        sys.exit("source has unreadable CONTENT tables; refusing to build a baseline "
                 "that silently loses them:\n  " + "\n  ".join(fatal))
    skip = set(damaged)

    db.execute("BEGIN")
    for typ, name, sql in objs:
        if typ == "table" and name not in skip:
            db.execute(sql)

    copied = {}
    for t in tables:
        if t in purge or t in skip:
            continue
        n = db.execute(f'SELECT count(*) FROM s."{t}" NOT INDEXED').fetchone()[0]
        if n:
            db.execute(f'INSERT INTO "{t}" SELECT * FROM s."{t}"')
        copied[t] = n

    # Damaged tables are recreated empty from their own CREATE statement, which
    # only needs sqlite_master and never touches the broken b-tree. Must happen
    # before the index pass: their indexes reference the table.
    for t in sorted(skip):
        row = db.execute("SELECT sql FROM s.sqlite_master WHERE type='table' AND name=?",
                         (t,)).fetchone()
        if row and row[0]:
            db.execute(row[0])

    for typ, name, sql in objs:
        if typ != "table":
            db.execute(sql)

    db.execute("CREATE TABLE IF NOT EXISTS version_info (version INTEGER DEFAULT 0)")
    db.execute("DELETE FROM version_info")
    db.execute("INSERT INTO version_info (version) VALUES (?)", (version,))
    db.commit()
    db.execute("DETACH DATABASE s")

    db.execute("VACUUM")
    db.execute("PRAGMA journal_mode=DELETE")
    db.commit()

    integrity = db.execute("PRAGMA integrity_check").fetchone()[0]

    problems = []
    for t in purge:
        if t in tables:
            n = db.execute(f'SELECT count(*) FROM "{t}"').fetchone()[0]
            if n:
                problems.append(f"{t} still has {n} row(s)")
    for t in KEEP_NONEMPTY:
        if t in tables:
            n = db.execute(f'SELECT count(*) FROM "{t}"').fetchone()[0]
            if n == 0:
                problems.append(f"{t} is empty -- content lost")
    db.close()

    if integrity != "ok" or problems:
        os.remove(tmp)
        sys.exit("refusing to ship a broken baseline: "
                 + (f"integrity_check={integrity}; " if integrity != "ok" else "")
                 + "; ".join(problems))

    os.replace(tmp, args.out)

    print(f"wrote {args.out}  version={version}")
    print(f"  {os.path.getsize(args.source)/1e6:.1f} MB -> "
          f"{os.path.getsize(args.out)/1e6:.1f} MB")
    print(f"  {len(tables)} tables in schema, {len(purge)} emptied, "
          f"{sum(copied.values())} content/config rows kept")
    if skip:
        print(f"  ! damaged in the source, recreated empty: {', '.join(sorted(skip))}")
    print("  integrity_check ok")
    for t in ("asm_data_set_devices", "asm_data_set_effect_groups", "map_game_info",
              "map_object_config", "ga_queues", "ga_map_pool_entries"):
        if t in copied:
            print(f"    {copied[t]:>8}  {t}")


if __name__ == "__main__":
    main()
