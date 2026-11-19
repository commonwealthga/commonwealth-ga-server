# Migrations

Forward-only schema/data changes applied by the **control server** at startup.

These files are **compiled into the binary**. `scripts/embed_migrations.py` runs
from the Makefile and regenerates
`src/ControlServer/Database/EmbeddedMigrations.gen.cpp` (gitignored) whenever the
contents change, so nothing has to be deployed alongside the binary — only
`dbbase/base.db` stays a file, and only for a database that does not exist yet.
Adding or editing a migration therefore needs a rebuild, which it needed anyway.

The control server is the only process that migrates a database. The game DLL
never does — it only reads `Schema::kVersion` and warns when the database it was
handed is older than the code expects.

## How it works

1. A database with **no schema at all** is replaced by `dbbase/base.db`. That
   baseline is generated from a live database by `scripts/make_dbbase.py`, with
   all player and match data stripped, so it is the state after every migration
   that had ever run when it was cut.
2. Then every `NNNN-<slug>.sql` here whose `NNNN` is **above** the recorded
   `version_info.version` is applied, ascending. Each file runs inside one
   transaction: it either lands completely or not at all. "Here" means compiled
   into the binary at build time — see the top of this file.
3. `version_info.version` is stamped to the file's `NNNN` on success.

## Writing one

- Name it `NNNN-slug.sql`, where `NNNN` is the version the database is at
  **after** the file runs. The baseline is `173`, so the next migration is
  `174-something.sql`. Pad to four digits; the prefix sets the order.
- Bump `Schema::kVersion` in `src/Shared/SchemaVersion.hpp` to your `NNNN` in the
  same commit. It is the number the game DLL checks before trusting a database.
- Files are replayed only once, so they do not need to be idempotent — but
  prefer `INSERT OR IGNORE` over `INSERT OR REPLACE` when seeding defaults.
  `INSERT OR REPLACE` with a partial column list **resets every column it does
  not mention**, which is how a curated `map_game_info.is_pvp = 1` once got
  silently reverted to `0` and made PvP maps classify themselves as PvE.
- Never touch `version_info` yourself; the runner owns it.
- `ALTER TABLE ... ADD COLUMN` for a column that already exists is **tolerated**
  and skipped (SQLite reports `duplicate column name` during prepare; the runner
  logs it and resumes at the next statement). That is deliberate: databases in
  the wild report the same version with different actual columns, so a plain
  `ALTER` must not abort an upgrade. Do not rely on it for anything else.
- A database already past your `NNNN` never sees the file. If you need a change
  to apply to existing databases too, write it idempotently and target the
  current baseline version + 1.

## Regenerating the baseline

```
python3 scripts/make_dbbase.py            # server.db -> dbbase/base.db
```

Run that after a batch of migrations has landed and been verified on a real
server: it folds their effects into the baseline, keeps the file small (player
data stripped and vacuumed), and refuses to write output if a content table would
come out empty or the result fails `integrity_check`.
