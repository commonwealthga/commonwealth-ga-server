# Database bootstrap & migrations

Read this before adding a table, a column, or a `Database::Init` block. The rules
here replaced roughly 15,000 lines of migration code and two competing version
counters; re-deriving them the hard way cost several sessions.

## The contract

**One writer.** The control server migrates the database. Nothing else does.

**One baseline.** `dbbase/base.db` is a complete schema plus all content/config
data with every player and match row stripped. A database with no schema at all
is replaced by it (SQLite backup API, one bulk copy).

**Forward-only, compiled in.** `migrations/NNNN-slug.sql`, ascending, each in one
transaction, stamped into `version_info.version`. They are *not* deployed as
files: `scripts/embed_migrations.py` turns them into
`Database/EmbeddedMigrations.gen.cpp` at build time (a `$(shell ...)` line in the
Makefile, writing only when contents change), so a deployed control server cannot
be started without its migrations. See `migrations/README.md`.

**One version.** `Schema::kVersion` in `src/Shared/SchemaVersion.hpp`. The game
DLL never migrates; it reads that constant only to warn when it was handed an
older database. Bump it in the same commit as any new migration.

Startup on the control server:

```
Migrations::Run(db, db_path, base_db)
  ├─ no tables in the file?        → copy dbbase/base.db over it
  ├─ read version_info.version
  ├─ embedded migrations where version > NNNN, ascending, one txn each
  └─ log: installed / applied N (a -> b) / up to date
```

`dbbase/base.db` stays a runtime file because it is a 7 MB database, not a script;
it is needed only when the database does not exist yet. Everything incremental
lives in the binary.

## Why it is like this

The DLL (`src/Database/Database.cpp`) and the control server
(`src/ControlServer/Database/Database.cpp`) both used to migrate the *same*
database through the *same* `version_info` counter. The DLL targeted 173, the
control server floor-wrote 19. A database created by the control server therefore
started at 19, and the DLL's entire ladder from 20 to 173 replayed against it
every time an instance started.

Replaying was not harmless. Blocks are written as seeds, e.g.

```sql
INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, ...)
VALUES (1131, 'Ticket_HimLab_4v4', 'TgGame.TgGame_PointRotation', ...);
```

`INSERT OR REPLACE` with a partial column list **deletes and reinserts**, so every
column the statement does not mention returns to its schema default. On a node
that reset `map_game_info.is_pvp` from 1 to 0, which made a PvP map report
`IsPvEMission() == true`, which made `TgDeploy_Beacon.PickUpDeployable` (which
refuses pickup in PvE) silently return false. A beacon that "didn't work on a
node" was a database versioning bug three layers away.

That is why the baseline exists: it is the *result* of every migration, applied
once, so no ladder ever replays.

## Rules for new work

- **Never add migration code to `Database::Init()`** in either process. Both are
  now short by design; the DLL's is 190 lines.
- **Seeds must be non-destructive.** Prefer `INSERT OR IGNORE`. If you truly need
  to change an existing row, name every column you are changing and use
  `UPDATE ... WHERE`, never a partial-column `INSERT OR REPLACE`.
- **Assume the baseline already has what you need.** Before adding DDL, check:
  `sqlite3 dbbase/base.db "PRAGMA table_info(ga_characters);"`.
- **Regenerate the baseline after a batch of migrations lands** and has been
  verified on a real server: `python3 scripts/make_dbbase.py`. It refuses to
  write output if a content table would come out empty or `integrity_check` fails.
- **Derived databases are disposable.** A node's `node.db` is content snapshot
  plus player bundles. If its version is below the baseline, delete the file and
  let it reinstall; do not hand-migrate it.

## Schema drift between processes

Even with one writer, a node can carry a database built at a different time than
the master's. `ContentSnapshot` therefore also exports every `ga_*` table
(except the three match-history tables) as **schema only**, and the node's
`NodeDataStore::SyncContent` converges to it: `CREATE TABLE IF NOT EXISTS` plus
`ALTER TABLE ADD COLUMN` for anything the master has and the node lacks. Columns
are added, never dropped.

## Corrupt sources

`scripts/make_dbbase.py` copies forward table by table rather than deleting rows
from a copy, so damage confined to one table cannot poison the baseline. It
full-scans every source table (`count(*) ... NOT INDEXED`) first: a damaged
*source* table in the purge list is recreated empty, and a damaged content table
aborts the run. When this was written, `server.db` had malformed b-tree pages
inside `ga_match_events` and `idx_ga_match_events_actor` — the largest table, and
purged anyway. If a future source reports damage, check `PRAGMA quick_check(20)`
before assuming the generator is at fault.
