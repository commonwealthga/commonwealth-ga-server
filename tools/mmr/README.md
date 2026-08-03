# MMR analysis tools

Offline scripts behind the performance-based rating engine in
`src/ControlServer/MmrService/`. None of them touch the server; they read a DB
snapshot and, in one case, write to it. Python 3, stdlib only.

Point them at a snapshot with the match tables populated. `reseed.py` takes the
path as an argument; the rest have it near the top of the file.

## The one that matters

**`reseed.py`** — a deliberate line-for-line port of `MmrService.cpp`, not a
tidier rewrite. It is the **acceptance test for the build**: reseed with the
compiled engine and with this, and the two should agree. If they don't, one of
them has a bug and this tells you what the answer should have been.

```
python reseed.py path/to/snapshot.db            # dry run, prints the fold summary
python reseed.py path/to/snapshot.db --apply    # rebuilds ga_mmr_history
```

Keep it in step with the C++. If you change a weight, a rule or a tunable in
`ClassProfiles.cpp`, change it here too or the acceptance test is worthless.

## Why the constants are what they are

Every number in `ClassProfiles.hpp` came from one of these. Re-run the relevant
script before changing one.

| script | question it answers |
|---|---|
| `closed_pool.py` | Can win/loss carry a rating at this population size? Shuffles which players were on the winning side, keeping every real roster and result, to get the win-rate spread chance alone would produce. **Observed sd 8.7pp against 6.5pp from chance — only ~45% of the variance is skill.** This is why win/loss was abandoned rather than retuned. |
| `wl_divisor.py` | Would retuning the old engine's Elo divisor fix it? Sweeps the divisor, and separately lets the update and the prediction use different values. Best single value is ~300 for a gain inside noise; the two uses pull in opposite directions because one compares an individual to a team and the other a team to a team. |
| `converge.py` | Does the rating settle? Compares the drifting rule against the anchored one and sweeps `perf_scale`. A drifting +0.56-performance player passes 5,000 by their thousandth game; anchored, they settle around 1,230. |
| `wl_project.py` | Forward-projects the old engine under both divisors, with resting points. |
| `team_splits.py` | Time-weighted headcount balance per match. **47% of matches are even, 51% are exactly one player apart** — which is what the 110-Elo headcount correction exists for. |
| `gen_data.py` | Builds the standings and per-player stat profiles used by the published pages. |
| `recent_form.py` | Rebuilds ratings from a recent window only (last two Sundays), for a form table. |
| `final_data.py` | Multi-class player profiles, plus past steamrolls with what the balancer would have done to them. |

## Things worth knowing before trusting any of it

- Everything rests on **158 rated matches**. The direction of each finding held
  across every cut; the exact constants did not. Several improvements measured
  are inside noise individually.
- Two of the three most consequential bugs were found by reviewers asking plain
  questions about a specific player, not by any test in here.
- The rating measures what is in `ga_match_player_stats`. Shot-calling,
  positioning, keeping the right player alive — none of it appears, and no
  reweighting of the existing columns will find it.
- `rep_points`, `objective_captures` and `beacon_spawns_provided` are populated
  as zero on every row. `capture_seconds` and `contest_seconds` carry real data
  but nothing reads them.
