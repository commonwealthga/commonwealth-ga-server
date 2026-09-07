#pragma once
//
// FairnessLog — append-only ga_matchmaking_fairness_events access
// (.planning/2026-09-07-matchmaking-fairness-log-design.md §4-5).
// IMPURE (sqlite): never link this into tests/matchmaking.
//
#include "src/ControlServer/MatchmakingService/Domain.hpp"

#include <cstdint>
#include <string>

namespace FairnessLog {

enum class Event { Excluded, Played, Defender };

// Append one row. instance_id <= 0 is stored as NULL (exclusions have no
// match). No-op when scope is empty or user_id <= 0.
void Record(const std::string& scope, uint32_t queue_id, int64_t user_id,
            Event e, uint32_t profile_id, int64_t instance_id);

// Read this user's rotation state for the scope. All-zero when they have no
// history, or when scope is empty / user_id <= 0.
FairnessStats Load(const std::string& scope, int64_t user_id);

}  // namespace FairnessLog
