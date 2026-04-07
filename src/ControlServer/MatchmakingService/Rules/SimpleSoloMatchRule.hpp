#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"

// SimpleSoloMatchRule -- pops immediately when any player is queued.
// Spawns a new instance with a hardcoded map. One instance per player.

class SimpleSoloMatchRule : public MatchRule {
public:
    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedPlayer>& players,
        const std::vector<RunningInstance>& instances) override
    {
        if (players.empty()) return std::nullopt;

        MatchResult result;
        result.map_name = "Rot_Redistribution05";
        result.game_mode = "TgGame.TgGame_PointRotation";
        for (const auto& p : players) {
            result.session_guids.push_back(p.session_guid);
            result.task_force_assignments[p.session_guid] = 1;
        }
        return result;
    }
};
