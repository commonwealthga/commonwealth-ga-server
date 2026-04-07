#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <mutex>
#include <vector>
#include <utility>

// InstanceRegistry.hpp -- Tracks known game instances in the ga_instances SQLite table.
// One row per running (or recently stopped) UE3 game server process.

struct InstanceInfo {
    int64_t     id           = 0;
    std::string map_name;
    std::string game_mode;
    std::string state;          // "STARTING" | "READY" | "DRAINING" | "STOPPED"
    int         pid           = 0;
    uint16_t    udp_port      = 0;
    std::string ip_address    = "127.0.0.1";
    int         player_count  = 0;
    int64_t     started_at    = 0;
    int64_t     sealed_at     = 0;  // 0 if not sealed
    int64_t     instance_id   = 0;  // unique ID assigned at spawn time (rowid)
    bool        is_home_map   = false; // true if spawned for home map
    int         max_players   = 0;  // filled by INSTANCE_READY
};

class InstanceRegistry {
public:
    // No-op -- mutex is default-constructed; here for symmetry with PlayerSessionStore.
    static void Init();

    // Set the external IP that instances are reachable at (from config).
    static void SetHost(const std::string& host);


    // Return the first READY instance for the given map name, or nullopt if none.
    static std::optional<InstanceInfo> GetReadyInstance(const std::string& map_name);

    // Mark all non-STOPPED instances from a previous run as STOPPED, then insert
    // a fresh READY row for the home map at the given UDP port.
    static void SeedHomeMapInstance(const std::string& map_name, uint16_t udp_port);

    // Insert a STARTING instance. Returns the SQLite rowid (used as instance_id).
    static int64_t InsertStarting(const std::string& map_name, const std::string& game_mode,
                                   uint16_t udp_port, int pid, bool is_home_map);

    // Store the real PID after the process has been spawned.
    static void UpdatePid(int64_t instance_id, int pid);

    // Transition STARTING -> READY when INSTANCE_READY IPC arrives.
    static void MarkReady(int64_t instance_id, int max_players);

    // Find first READY instance with is_home_map=1.
    static std::optional<InstanceInfo> GetReadyHomeInstance();

    // Look up any instance by instance_id (any state).
    static std::optional<InstanceInfo> GetInstanceById(int64_t instance_id);

    // Mark an instance STOPPED (IPC disconnect or crash).
    static void MarkStopped(int64_t instance_id);

    // Get next available UDP port from range. Returns nullopt if pool exhausted.
    static std::optional<uint16_t> AllocatePort(uint16_t lo, uint16_t hi);

    // Clear all non-STOPPED instances (startup recovery).
    static void ClearStaleInstances();

    static void InsertInstancePlayer(int64_t instance_id, const std::string& session_guid,
                                     int64_t character_id, int task_force);
    static void MarkInstancePlayerLeft(int64_t instance_id, const std::string& session_guid);
    static void MarkAllInstancePlayersLeft(int64_t instance_id);
    static std::pair<int, int> GetTeamCounts(int64_t instance_id);
    static int GetActivePlayerCount(int64_t instance_id);
    static std::vector<InstanceInfo> GetReadyMissionInstances();
    static std::optional<InstanceInfo> GetHomeInstance();
    static std::vector<InstanceInfo> GetIdleInstances(int timeout_seconds);
    static void SetLastEmptyAtIfEmpty(int64_t instance_id);

private:
    static std::mutex mutex_;
    static std::string s_host_;
};
