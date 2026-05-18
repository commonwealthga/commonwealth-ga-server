#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"

// SimplePvPMatchRule -- joins any seat-available instance in this queue
// (regardless of which map it landed on), balancing players onto the smaller
// team. If no joinable instance exists, signals "spawn new" by returning a
// result with an empty map_name -- the matchmaker fills it from the queue's
// map_pool at random.

class SimplePvPMatchRule : public MatchRule {
public:
    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedPlayer>& players,
        const std::vector<RunningInstance>& instances) override
    {
        if (players.empty()) return std::nullopt;

        // `instances` is already queue-filtered by the provider in main.cpp.
        // Any READY instance in this queue is a valid join target (matching
        // the original behavior — no seat cap; the game decides fills).
        for (const auto& inst : instances) {
            auto [team1, team2] = InstanceRegistry::GetTeamCounts(inst.instance_id);

            MatchResult result;
            result.map_name  = inst.map_name;
            result.game_mode = inst.game_mode;
            result.existing_instance_id = inst.instance_id;
            for (const auto& p : players) {
                int tf = (team1 <= team2) ? 1 : 2;
                result.session_guids.push_back(p.session_guid);
                result.task_force_assignments[p.session_guid] = tf;
                if (tf == 1) team1++; else team2++;
            }
            return result;
        }

        // No joinable instance — leave map/mode empty so TryPop picks from
        // the queue's map_pool. All players start on team 1; the next batch
        // joining mid-game will rebalance via the loop above.
        MatchResult result;
        for (const auto& p : players) {
            result.session_guids.push_back(p.session_guid);
            result.task_force_assignments[p.session_guid] = 1;
        }
        return result;
    }
};
