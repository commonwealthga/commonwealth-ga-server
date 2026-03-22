#include "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
#include "src/ControlServer/Logger.hpp"

// InstanceSpawner.cpp -- Stub. Full implementation added in Phase 9 Plan 02.

int64_t InstanceSpawner::Spawn(const ControlServerConfig& /*cfg*/,
                                const std::string& /*map_name*/,
                                const std::string& /*game_mode*/,
                                bool /*is_home_map*/) {
    Logger::Log("spawner", "[InstanceSpawner] Spawn not yet implemented (Phase 9 Plan 02)\n");
    return 0;
}
