#pragma once

#include <cstdint>

// Match-seed contract shared by the control server and the game DLL.
//
// A match seed only reproduces anything if BOTH sides agree on how it is
// derived and consumed. `kMatchSeedScheme` is bumped whenever a roll is added,
// removed, reordered or re-keyed (GameRng::Purpose values, per-decision
// ordinals, the mixing function). A stored seed therefore carries the scheme it
// was recorded under:
//
//   ga_instances.match_seed + ga_instances.seed_scheme
//
// The stats site replays a seed only when `seed_scheme` equals the scheme of
// the build that is about to run it — otherwise it should offer the seed as
// "viewable, not replayable" instead of silently producing a different match.
//
// Scheme history:
//   1 — keyed per-decision streams: spawn-plan roster, per-entry bot type,
//       spawn location + jitter, wave pacing/order, map-object variants,
//       PointRotation objective pick, boss-alarm factory pick.
constexpr int kMatchSeedScheme = 1;

// Seeds are stored in SQLite INTEGER (signed 64-bit), so keep them positive.
constexpr uint64_t kMatchSeedMax = 0x7FFFFFFFFFFFFFFFull;
