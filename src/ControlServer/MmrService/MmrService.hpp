#pragma once
#include <mutex>
#include <string>

// Live MMR rating engines (design .planning/2026-07-12-mmr-engine-design.md).
// Two engines fold every concluded PvP match, per (player, class):
//   wl   — pure win/loss Elo              -> ga_wl_mmr_history/_processed
//   perf — performance-adjusted Elo       -> ga_mmr_history/_processed
// cs_settings.active_mmr_engine ('wl'|'perf') selects which one downstream
// consumers (matchmaking, stats site) use.
class MmrService {
public:
    // Fold every concluded, unprocessed PvP match into both engines, in
    // conclusion-time order. Called at startup (initial seed / self-heal)
    // and after every MSG_MISSION_ENDED outcome write.
    static void FoldUnprocessed();

    // Wipe both engines' history + watermarks and replay everything.
    // Returns a human-readable summary for the admin response.
    static std::string Reseed();

    static std::string GetActiveEngine();                 // "wl" | "perf"
    static bool SetActiveEngine(const std::string& engine);

private:
    static std::mutex mutex_;
};
