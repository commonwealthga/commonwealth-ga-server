#pragma once

// Server performance switches (ga-h-perf, patches 1101-1102), read from the
// cs_settings table (key -> integer), re-read at most once a minute. Each one
// keeps the old behaviour at 0, so a rollback is a settings row, not a build.
//   perf_iscrewed_native  0 = TgPawn.IsCrewed runs as script (old)
//                         1 = answered natively, same expression (default)
//                         2 = verify: run both, log mismatches on "perf"
//   perf_aictrl_lod       0 = AI controllers think every frame (old)
//                         1 = AI controllers follow their pawn's distance LOD (default)
namespace PerfKnobs {

int IsCrewedNative();
int AiCtrlLod();

}  // namespace PerfKnobs
