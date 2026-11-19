#include "src/ControlServer/NodeControl/NodeControl.hpp"
#include "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeLink/NodeMessages.hpp"
#include "src/ControlServer/NodeRegistry/NodeRegistry.hpp"
#include "lib/nlohmann/json.hpp"

// NodeControl.cpp -- the seam between "an instance must exist" and "how".
//
// node_id 0 is this machine and goes straight to the fork/exec spawner, exactly
// as before. Anything else is dispatched to a remote node over the link; the
// master inserts the ga_instances row first (it owns the id), so the node mirrors
// a row that already exists.

namespace {

const char* DifficultyKind(eDifficulty kind) {
    switch (kind) {
        case eDifficulty::Id:     return "id";
        case eDifficulty::Scalar: return "scalar";
        case eDifficulty::Default:
        default:                  return "default";
    }
}

}  // namespace

std::optional<uint16_t> NodeControl::AllocatePort(const ControlServerConfig& cfg, int node_id) {
    if (node_id == 0) {
        return InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
    }
    if (!NodeRegistry::IsConfigured(node_id)) {
        Logger::Log("node", "[NodeControl] AllocatePort: node_id=%d is not configured\n", node_id);
        return std::nullopt;
    }
    // The node owns its own port space; 0 means "the node will pick one".
    return static_cast<uint16_t>(0);
}

SpawnOutcome NodeControl::Spawn(const ControlServerConfig& cfg, const SpawnRequest& req) {
    SpawnOutcome out;

    // An instance spawned without an id can never register: the DLL reads it from
    // -instanceid=, sends it in INSTANCE_HELLO, and IpcServer rejects 0. The
    // process then retries forever while the spawner, seeing no instance, tries
    // again -- one observed failure produced 41 game processes. Refuse here so no
    // caller can leak processes that could never have worked.
    if (req.instance_id <= 0) {
        out.error = "no instance_id (ga_instances insert failed?)";
        Logger::Log("main",
            "[NodeControl] Spawn refused for map=%s node=%d: %s\n",
            req.map_name.c_str(), req.node_id, out.error.c_str());
        return out;
    }

    if (req.node_id == 0) {
        const pid_t pid = InstanceSpawner::Spawn(cfg, req);
        if (pid < 0) {
            out.error = "local spawn failed";
            return out;
        }
        out.dispatched = true;
        out.pid = pid;
        return out;
    }

    std::string why_not;
    if (!NodeRegistry::CanAcceptWork(req.node_id, why_not)) {
        out.error = why_not;
        Logger::Log("node",
            "[NodeControl] Spawn: instance=%lld map=%s refused: %s\n",
            (long long)req.instance_id, req.map_name.c_str(), why_not.c_str());
        return out;
    }

    nlohmann::json j;
    j["type"]               = NodeMsg::kInstanceSpawn;
    j["instance_id"]        = req.instance_id;
    j["map_name"]           = req.map_name;
    j["game_mode"]          = req.game_mode;
    j["udp_port"]           = req.udp_port;   // 0 = node picks
    j["seed"]               = req.match_seed;
    j["difficulty_kind"]    = DifficultyKind(req.difficulty_type);
    j["difficulty_value_id"] = req.difficulty_value_id;
    // Same encoding the local -difficulty_override argument uses, so both paths
    // hand the DLL identical numbers.
    j["difficulty_hp"]      = static_cast<uint32_t>(req.difficulty_scalar.HP * 100.0f);
    j["difficulty_dmg"]     = static_cast<uint32_t>(req.difficulty_scalar.Dmg * 100.0f);
    j["queue_id"]           = req.queue_id;
    j["is_home_map"]        = req.is_home_map;
    // The node mirrors our ga_instances row. victory_bonus_lives is derived from
    // the queue at insert time and read by the DLL (InitGameRepInfo), and the
    // node's own database may not carry the queue row, so send the value.
    j["victory_bonus_lives"] = InstanceRegistry::GetVictoryBonusLives(req.instance_id);
    j["node_id"]             = req.node_id;

    if (!NodeRegistry::SendControl(req.node_id, j.dump())) {
        out.error = "could not send INSTANCE_SPAWN";
        Logger::Log("node",
            "[NodeControl] Spawn: instance=%lld map=%s could not reach node_id=%d\n",
            (long long)req.instance_id, req.map_name.c_str(), req.node_id);
        return out;
    }

    Logger::Log("node",
        "[NodeControl] dispatched instance=%lld map=%s seed=%llu to node '%s' (id=%d)\n",
        (long long)req.instance_id, req.map_name.c_str(),
        (unsigned long long)req.match_seed,
        NodeRegistry::NameOf(req.node_id).c_str(), req.node_id);

    out.dispatched = true;   // the node reports its pid back in INSTANCE_SPAWNING
    return out;
}

void NodeControl::Stop(const InstanceInfo& inst, const char* reason, int grace_seconds) {
    if (!IsRemote(inst)) {
        InstanceSpawner::StopInstanceProcess(inst, reason, grace_seconds);
        return;
    }

    if (!NodeRegistry::IsUp(inst.node_id)) {
        // The link is already gone, so the node cannot be told. Its instances
        // outlive the master until the node's own reaping stops them; say so
        // rather than pretending the stop happened.
        Logger::Log("node",
            "[NodeControl] Stop: instance=%lld lives on node_id=%d which is unreachable "
            "(reason=%s) -- cannot stop it from here\n",
            (long long)inst.instance_id, inst.node_id, reason ? reason : "unspecified");
        return;
    }

    nlohmann::json j;
    j["type"]          = NodeMsg::kInstanceStop;
    j["instance_id"]   = inst.instance_id;
    j["reason"]        = reason ? reason : "unspecified";
    j["grace_seconds"] = grace_seconds;

    if (!NodeRegistry::SendControl(inst.node_id, j.dump())) {
        Logger::Log("node",
            "[NodeControl] Stop: instance=%lld on node_id=%d: send failed\n",
            (long long)inst.instance_id, inst.node_id);
        return;
    }
    Logger::Log("node", "[NodeControl] asked node '%s' (id=%d) to stop instance=%lld (%s)\n",
        NodeRegistry::NameOf(inst.node_id).c_str(), inst.node_id,
        (long long)inst.instance_id, reason ? reason : "unspecified");
}

void NodeControl::StopLocally(const InstanceInfo& inst, const char* reason,
                              int grace_seconds) {
    InstanceSpawner::StopInstanceProcess(inst, reason, grace_seconds);
}

bool NodeControl::IsRemote(const InstanceInfo& inst) {
    return inst.node_id != 0;
}

bool NodeControl::EnsureSlotPrefixes(const ControlServerConfig& cfg) {
    return InstanceSpawner::EnsureSlotPrefixes(cfg);
}
