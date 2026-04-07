#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"

// SimplePvPMatchRule -- tries to join an existing instance with the same map/game_mode.
// Assigns player to the team with fewer players. Spawns new if no instance exists.

class SimplePvPMatchRule : public MatchRule {
public:
    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedPlayer>& players,
        const std::vector<RunningInstance>& instances) override
    {
        if (players.empty()) return std::nullopt;

        // Check for an existing READY mission instance with matching map
        auto mission_instances = InstanceRegistry::GetReadyMissionInstances();
        for (const auto& inst : mission_instances) {
            if (inst.map_name == "Rot_Redistribution05" &&
                inst.game_mode == "TgGame.TgGame_PointRotation") {
                // Found existing instance — route player there
                auto [team1, team2] = InstanceRegistry::GetTeamCounts(inst.instance_id);

                MatchResult result;
                result.map_name = inst.map_name;
                result.game_mode = inst.game_mode;
                result.existing_instance_id = inst.instance_id;
                for (const auto& p : players) {
                    int tf = (team1 <= team2) ? 1 : 2;
                    result.session_guids.push_back(p.session_guid);
                    result.task_force_assignments[p.session_guid] = tf;
                    // Update counts for next player in this batch
                    if (tf == 1) team1++; else team2++;
                }
                return result;
            }
        }

        // No existing instance — spawn new, all players start on team 1
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
