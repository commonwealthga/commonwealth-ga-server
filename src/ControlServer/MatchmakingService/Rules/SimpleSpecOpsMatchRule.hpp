#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"

// SimpleSpecOpsMatchRule -- joins any seat-available instance in this queue
// (regardless of which map it landed on), all players on attackers (team 1).
// If no joinable instance exists, signals "spawn new" by leaving map_name
// empty -- the matchmaker fills it from the queue's map_pool at random.

class SimpleSpecOpsMatchRule : public MatchRule {
public:
    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedPlayer>& players,
        const std::vector<RunningInstance>& instances) override
    {
        if (players.empty()) return std::nullopt;

        // `instances` is already queue-filtered by the provider in main.cpp.
        // No seat cap — matches original behavior; the game decides fills.
        for (const auto& inst : instances) {
            MatchResult result;
            result.map_name  = inst.map_name;
            result.game_mode = inst.game_mode;
            result.existing_instance_id = inst.instance_id;
            for (const auto& p : players) {
                result.session_guids.push_back(p.session_guid);
                result.task_force_assignments[p.session_guid] = 1;
            }
            return result;
        }

        // No joinable instance — empty map/mode signals "TryPop, pick from pool".
        MatchResult result;
        for (const auto& p : players) {
            result.session_guids.push_back(p.session_guid);
            result.task_force_assignments[p.session_guid] = 1;
        }
        return result;
    }
};
