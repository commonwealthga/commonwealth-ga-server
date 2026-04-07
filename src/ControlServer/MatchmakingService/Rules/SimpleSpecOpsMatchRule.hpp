#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"

// SimpleSpecOpsMatchRule -- tries to join existing instance, puts all players on attackers (team 1).

class SimpleSpecOpsMatchRule : public MatchRule {
public:
    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedPlayer>& players,
        const std::vector<RunningInstance>& instances) override
    {
        if (players.empty()) return std::nullopt;

        auto mission_instances = InstanceRegistry::GetReadyMissionInstances();
        for (const auto& inst : mission_instances) {
            if (inst.map_name == "Rot_Redistribution05" &&
                inst.game_mode == "TgGame.TgGame_PointRotation") {
                MatchResult result;
                result.map_name = inst.map_name;
                result.game_mode = inst.game_mode;
                result.existing_instance_id = inst.instance_id;
                for (const auto& p : players) {
                    result.session_guids.push_back(p.session_guid);
                    result.task_force_assignments[p.session_guid] = 1;
                }
                return result;
            }
        }

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
