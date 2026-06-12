#pragma once

#include <cstdint>
#include <map>

// Pure per-user stint banking over the PRI's cumulative r_Scores array.
// Design: .planning/2026-06-12-match-stats-tracking-design.md ("B. DLL —
// MatchStatsTracker"). No SDK types — unit-tested in tests/matchstats.
namespace Stats {

constexpr int kNumScores = 11;  // mirrors ATgRepInfo_Player::r_Scores[0xB]

struct StintKey {
    int64_t character_id = 0;
    int     task_force   = 0;
    bool operator<(const StintKey& o) const {
        if (character_id != o.character_id) return character_id < o.character_id;
        return task_force < o.task_force;
    }
    bool operator==(const StintKey& o) const {
        return character_id == o.character_id && task_force == o.task_force;
    }
};

struct StintStats {
    int   scores[kNumScores] = {};
    float capture_seconds        = 0.0f;
    float contest_seconds        = 0.0f;
    int   objective_captures     = 0;
    int   beacon_spawns_provided = 0;
    int   beacon_spawns_used     = 0;
    int   beacons_destroyed      = 0;
    float seconds_played         = 0.0f;
};

// One per user per match. The PRI's r_Scores is one cumulative array per
// connection; stints split it per (character, task_force) via baseline
// deltas. Reconnect restore = sum of banked scores written back into the
// fresh PRI, with the baseline re-anchored so nothing double-counts.
class UserMatchStats {
public:
    // Open a stint (banks the previous one first if open). `pri_scores`
    // is the PRI's CURRENT r_Scores — it becomes the new baseline.
    void BeginStint(const StintKey& key, const int pri_scores[kNumScores],
                    float game_time);

    // Fold (pri - baseline) into the open stint, advance the baseline,
    // accumulate seconds_played. No-op when no stint is open.
    void Bank(const int pri_scores[kNumScores], float game_time);

    // Bank + close (used on leave).
    void CloseStint(const int pri_scores[kNumScores], float game_time);

    // Sum of banked scores across all stints (reconnect restore).
    void SumScores(int out[kNumScores]) const;

    // After the caller writes restored sums into a fresh PRI, re-anchor
    // the open stint's baseline to those values.
    void RebaselineAfterRestore(const int pri_scores[kNumScores]);

    bool HasOpenStint() const { return has_open_; }
    bool HasAnyStint()  const { return has_open_ || !stints_.empty(); }
    const StintKey& OpenKey() const { return open_key_; }

    // Live accumulator target (capture seconds etc.). nullptr if closed.
    StintStats* OpenStint();

    const std::map<StintKey, StintStats>& Stints() const { return stints_; }

private:
    std::map<StintKey, StintStats> stints_;
    StintKey open_key_;
    int      baseline_[kNumScores] = {};
    float    stint_started_ = 0.0f;
    bool     has_open_ = false;
};

}  // namespace Stats
