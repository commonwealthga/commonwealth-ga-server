#pragma once

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/Config/DifficultyScalar.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include <string>
#include <optional>
#include <cstdint>
#include <sys/types.h>

// NodeControl.hpp -- the single seam for "make a game instance exist" and
// "make it stop".
//
// Every spawn, stop and UDP-port allocation in the control server goes through
// this facade rather than calling InstanceSpawner directly, so no caller has to
// know whether an instance runs on this machine or on another one.
//
// Phase 0 has exactly one implementation: the local one, which forwards to
// InstanceSpawner. Phase 1 adds the remote implementation that turns Spawn()
// into an INSTANCE_SPAWN message over the node link and Stop() into
// INSTANCE_STOP. Calls that name a remote node are refused loudly today
// instead of silently spawning locally.

// How an instance's difficulty is expressed. Id = a difficulty_value_id from
// the data set; Scalar = an explicit HP/damage multiplier pair.
enum class eDifficulty { Default, Id, Scalar };

struct SpawnRequest {
    std::string map_name;
    std::string game_mode;
    uint16_t    udp_port            = 0;
    int64_t     instance_id         = 0;
    eDifficulty difficulty_type     = eDifficulty::Default;
    uint32_t    difficulty_value_id = 0;
    DifficultyScalar difficulty_scalar = DifficultyScalar();
    uint64_t    match_seed          = 0;
    // Owning node: 0 = this machine. The master is node 0, so a single-machine
    // deployment never sets this.
    int         node_id             = 0;
    // Carried to a remote node so its mirrored ga_instances row is complete.
    // Ignored by the local spawner, which derives it from the queue.
    uint32_t    queue_id            = 0;
    bool        is_home_map         = false;
};

// What a Spawn() call actually achieved. Spawning on another machine is
// asynchronous: success means "handed to a node", not "process running". The real
// outcome arrives later as INSTANCE_SPAWNING (port + pid) or INSTANCE_FAILED.
struct SpawnOutcome {
    bool        dispatched = false;  // accepted for spawn, locally or remotely
    pid_t       pid        = 0;      // local pid; 0 when the node owns the process
    std::string error;               // why it was not dispatched
};

class NodeControl {
public:
    // Next free UDP port for an instance about to be spawned on `node_id`.
    //
    // A local node scans ga_instances. A remote node owns its own port space, so
    // the master cannot know the port yet: it returns 0 (engaged, meaning "the
    // node will pick"), and InstanceRegistry::UpdateUdpPort fills in the real
    // value from INSTANCE_SPAWNING -- which always arrives before the instance can
    // become READY, so no GO_PLAY can ever carry port 0.
    static std::optional<uint16_t> AllocatePort(const ControlServerConfig& cfg, int node_id = 0);

    // Spawn on the requested node (0 = this machine).
    static SpawnOutcome Spawn(const ControlServerConfig& cfg, const SpawnRequest& req);

    // Stop the process group of an already-spawned instance. Does not update
    // InstanceRegistry state; callers keep doing that themselves.
    //
    // Dispatches on inst.node_id, so it refuses anything that is not local. That is
    // right for the master and WRONG for a node: a node's mirrored rows carry the
    // master's node id, so its own instances look remote to this function.
    static void Stop(const InstanceInfo& inst, const char* reason, int grace_seconds = 3);

    // Stop a process this machine owns, ignoring ga_instances.node_id. A node
    // always owns the processes it forked, whatever node id the mirrored row
    // records, so every stop on the node side uses this.
    static void StopLocally(const InstanceInfo& inst, const char* reason,
                            int grace_seconds = 3);

    // True when the instance is hosted by another node, i.e. nobody here can
    // signal its pid and ga_instances.pid refers to a process this machine does
    // not own.
    static bool IsRemote(const InstanceInfo& inst);

    // Owning node id of an instance (0 = this machine). Reads ga_instances via the
    // struct the caller already has.
    static int NodeIdOf(const InstanceInfo& inst) { return inst.node_id; }

    // Provision per-slot WINEPREFIXes for the local node. Idempotent; call once
    // at startup before the first Spawn().
    static bool EnsureSlotPrefixes(const ControlServerConfig& cfg);
};
