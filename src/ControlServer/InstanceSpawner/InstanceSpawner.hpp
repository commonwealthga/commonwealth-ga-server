#pragma once

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include <string>
#include <cstdint>
#include <sys/types.h>

struct InstanceInfo;

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

    // Terminate the process group for an already-spawned game instance.
    // Sends SIGTERM immediately, then SIGKILL after grace_seconds if the
    // process group still exists. Does not update InstanceRegistry state.
    static void StopInstanceProcess(const InstanceInfo& inst,
                                    const char* reason,
                                    int grace_seconds = 3);

    // Number of round-robin slots derivable from cfg.game_cpu_range and
    // cfg.cores_per_instance. 0 if pinning is disabled. Used for both
    // affinity assignment and per-slot WINEPREFIX provisioning.
    static int SlotCount(const ControlServerConfig& cfg);

    // Path of the WINEPREFIX for a given slot. Empty if pinning disabled.
    static std::string SlotPrefixPath(const ControlServerConfig& cfg, int slot_idx);

    // If cfg.per_slot_prefix is on, ensure each slot has its own
    // WINEPREFIX cloned from the base. Idempotent — skips slots whose
    // prefix dir already exists. Returns false on irrecoverable error.
    // Call once at control-server startup, before any Spawn().
    static bool EnsureSlotPrefixes(const ControlServerConfig& cfg);
};
