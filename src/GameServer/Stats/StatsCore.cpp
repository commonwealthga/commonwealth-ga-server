#include "src/GameServer/Stats/StatsCore.hpp"

namespace Stats {

void UserMatchStats::BeginStint(const StintKey& key,
                                const int pri_scores[kNumScores],
                                float game_time) {
    if (has_open_) Bank(pri_scores, game_time);
    open_key_ = key;
    for (int i = 0; i < kNumScores; i++) baseline_[i] = pri_scores[i];
    stint_started_ = game_time;
    has_open_ = true;
    stints_[key];  // ensure the row exists even before the first Bank
}

void UserMatchStats::Bank(const int pri_scores[kNumScores], float game_time) {
    if (!has_open_) return;
    StintStats& s = stints_[open_key_];
    for (int i = 0; i < kNumScores; i++) {
        s.scores[i] += pri_scores[i] - baseline_[i];
        baseline_[i] = pri_scores[i];
    }
    if (game_time > stint_started_) {
        s.seconds_played += game_time - stint_started_;
    }
    stint_started_ = game_time;
}

void UserMatchStats::CloseStint(const int pri_scores[kNumScores],
                                float game_time) {
    Bank(pri_scores, game_time);
    has_open_ = false;
}

void UserMatchStats::SumScores(int out[kNumScores]) const {
    for (int i = 0; i < kNumScores; i++) out[i] = 0;
    for (const auto& kv : stints_) {
        for (int i = 0; i < kNumScores; i++) out[i] += kv.second.scores[i];
    }
}

void UserMatchStats::RebaselineAfterRestore(const int pri_scores[kNumScores]) {
    if (!has_open_) return;
    for (int i = 0; i < kNumScores; i++) baseline_[i] = pri_scores[i];
}

StintStats* UserMatchStats::OpenStint() {
    if (!has_open_) return nullptr;
    return &stints_[open_key_];
}

}  // namespace Stats
