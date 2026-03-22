#pragma once

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include <string>
#include <cstdint>
#include <sys/types.h>

// InstanceSpawner.hpp -- Spawns UE3 game server processes via fork/exec with Wine.

class InstanceSpawner {
public:
    // Spawn a UE3 game instance via fork/exec with Wine.
    // Returns child PID on success, -1 on failure.
    static pid_t Spawn(const ControlServerConfig& cfg,
                       const std::string& map_name,
                       const std::string& game_mode,
                       uint16_t udp_port,
                       int64_t instance_id);
};
