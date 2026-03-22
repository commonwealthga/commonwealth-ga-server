#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <mutex>

// InstanceRegistry.hpp -- Tracks known game instances in the ga_instances SQLite table.
// One row per running (or recently stopped) UE3 game server process.

struct InstanceInfo {
    int64_t     id           = 0;
    std::string map_name;
    std::string state;          // "STARTING" | "READY" | "DRAINING" | "STOPPED"
    int         pid           = 0;
    uint16_t    udp_port      = 0;
    std::string ip_address    = "127.0.0.1";
    int         player_count  = 0;
    int64_t     started_at    = 0;
    int64_t     sealed_at     = 0;  // 0 if not sealed
};

class InstanceRegistry {
public:
    // No-op -- mutex is default-constructed; here for symmetry with PlayerSessionStore.
    static void Init();

    // Return the first READY instance for the given map name, or nullopt if none.
    static std::optional<InstanceInfo> GetReadyInstance(const std::string& map_name);

    // Mark all non-STOPPED instances from a previous run as STOPPED, then insert
    // a fresh READY row for the home map at the given UDP port.
    static void SeedHomeMapInstance(const std::string& map_name, uint16_t udp_port);

private:
    static std::mutex mutex_;
};
