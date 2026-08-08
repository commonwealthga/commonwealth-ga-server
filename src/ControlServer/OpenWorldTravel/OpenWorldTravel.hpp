#pragma once

#include <cstdint>
#include <functional>
#include <string>

// OpenWorldTravel -- routes a player to a persistent open-world zone when they
// Use a Map Transition omega volume (asm_data_set_ui_volumes.volume_type_value_id
// = 1255). The DLL sends REQUEST_TRAVEL with the destination map_game_id; this
// resolves it, guarantees exactly one shared instance per map, and hands the
// session to TcpSession for PLAYER_REGISTER + GSC_GO_PLAY.
//
// Eligibility is data-driven: map_game_info.gameplay_type_value_id must be
// 1554 ("PVE- Open Zone"). Queue maps and mission maps are rejected outright,
// so a mis-authored volume can't be used to teleport into a mission.
class OpenWorldTravel {
public:
    // Spawner hook. Registered once from main.cpp (it owns the config + port
    // pool). Returns the new instance_id, or 0 on failure.
    using SpawnFn = std::function<int64_t(const std::string& map_name,
                                          const std::string& game_mode,
                                          uint32_t difficulty_value_id)>;
    static void SetSpawner(SpawnFn fn);

    // Entry point for MSG_REQUEST_TRAVEL. Safe to call repeatedly for the same
    // player and from several players at once — at most one instance per map
    // is ever spawned. Silent (logged) on every rejection path; the player
    // simply stays where they are.
    static void Request(const std::string& session_guid, uint32_t map_game_id);

    // Task force open-world arrivals are placed on. Defenders — these maps have
    // no attacking side.
    // TODO: per-map config for volume-spawned maps. Queue-spawned maps get
    // their side/difficulty from ga_queues; volume-spawned maps currently have
    // nowhere to express it, so both are hardcoded here.
    static constexpr int kTaskForce = 2;

    // ULTRA_MAX_SECURITY (ga_queues.difficulty_value_id used by the umax /
    // desert_raids queues). Same TODO as above.
    static constexpr uint32_t kDifficultyValueId = 1471;
};
