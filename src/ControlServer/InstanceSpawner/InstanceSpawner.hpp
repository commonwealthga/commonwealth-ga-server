#pragma once

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include <cstdint>

// InstanceSpawner.hpp -- Spawns UE3 game server processes via fork/exec.
// Implemented in Phase 9 Plan 02.

class InstanceSpawner {
public:
    // Spawn a new game instance for the given map.
    // Returns the instance_id (SQLite rowid) on success, or 0 on failure.
    static int64_t Spawn(const ControlServerConfig& cfg,
                         const std::string& map_name,
                         const std::string& game_mode,
                         bool is_home_map);
};
