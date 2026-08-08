#include "src/ControlServer/OpenWorldTravel/OpenWorldTravel.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"

#include <mutex>
#include <set>

namespace {
    OpenWorldTravel::SpawnFn g_spawner;

    // Guards the check-then-spawn sequence. Everything runs on the single io
    // thread today, but the DB read + spawn is a compound operation and the
    // whole point of this path is that it must never spawn twice.
    std::mutex           g_mutex;
    std::set<std::string> g_spawning;   // map names with a spawn in flight
}

void OpenWorldTravel::SetSpawner(SpawnFn fn) {
    g_spawner = std::move(fn);
}

void OpenWorldTravel::Request(const std::string& session_guid, uint32_t map_game_id) {
    if (session_guid.empty() || map_game_id == 0) {
        Logger::Log("travel", "[OpenWorldTravel] Ignoring request: guid='%s' map_game_id=%u\n",
            session_guid.c_str(), map_game_id);
        return;
    }

    auto row = MapGameInfo::LookupByMapGameId(map_game_id);
    if (!row) {
        Logger::Log("travel",
            "[OpenWorldTravel] map_game_id=%u has no map_game_info row — no destination (guid=%s)\n",
            map_game_id, session_guid.c_str());
        return;
    }
    if (row->gameplay_type_value_id != MapGameInfo::kGameplayTypeOpenZone) {
        Logger::Log("travel",
            "[OpenWorldTravel] map_game_id=%u ('%s') gameplay_type=%u is not an open zone (%u) — refusing (guid=%s)\n",
            map_game_id, row->map_name.c_str(), row->gameplay_type_value_id,
            MapGameInfo::kGameplayTypeOpenZone, session_guid.c_str());
        return;
    }
    if (row->map_name.empty() || row->game_class.empty()) {
        Logger::Log("travel",
            "[OpenWorldTravel] map_game_id=%u has an incomplete row (map_name='%s' game_class='%s')\n",
            map_game_id, row->map_name.c_str(), row->game_class.c_str());
        return;
    }

    // Single shared instance per map. A STARTING row already counts as live,
    // so once the first Use has inserted one, every subsequent Use — from this
    // player or any other — falls through to the wait-and-register path
    // instead of spawning again.
    bool spawn_here = false;
    bool warmed_up  = false;   // a READY instance exists — the player travels immediately
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto live = InstanceRegistry::GetLiveInstanceByMapName(row->map_name);
        if (live) {
            warmed_up = (live->state == "READY");
            Logger::Log("travel",
                "[OpenWorldTravel] '%s' already live: instance=%lld state=%s — %s %s\n",
                row->map_name.c_str(), (long long)live->instance_id, live->state.c_str(),
                session_guid.c_str(), warmed_up ? "travels now" : "will wait for READY");
        } else if (g_spawning.count(row->map_name)) {
            Logger::Log("travel",
                "[OpenWorldTravel] '%s' spawn already in flight — %s will wait for it\n",
                row->map_name.c_str(), session_guid.c_str());
        } else if (!g_spawner) {
            Logger::Log("travel", "[OpenWorldTravel] No spawner registered — cannot start '%s'\n",
                row->map_name.c_str());
            return;
        } else {
            g_spawning.insert(row->map_name);
            spawn_here = true;
        }
    }

    if (spawn_here) {
        Logger::Log("travel", "[OpenWorldTravel] Spawning '%s' (%s) difficulty=%u for %s\n",
            row->map_name.c_str(), row->game_class.c_str(), kDifficultyValueId,
            session_guid.c_str());
        const int64_t instance_id = g_spawner(row->map_name, row->game_class, kDifficultyValueId);
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_spawning.erase(row->map_name);
        }
        if (instance_id == 0) {
            Logger::Log("travel", "[OpenWorldTravel] Spawn of '%s' failed — dropping travel for %s\n",
                row->map_name.c_str(), session_guid.c_str());
            return;
        }
        Logger::Log("travel", "[OpenWorldTravel] '%s' spawning as instance=%lld\n",
            row->map_name.c_str(), (long long)instance_id);
    }

    // Cold zone — the player is about to sit on a poll for as long as the map
    // takes to load. Tell them so, on their current instance (they're still in
    // it). Player-scoped: DeliverPlayerAction targets one session_guid.
    if (!warmed_up) {
        nlohmann::json action;
        action["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
        action["session_guid"] = session_guid;
        action["action"]       = "alert_text";
        action["args"] = {
            {"text",     "Instance is starting, please wait..."},
            {"priority", 2},     // APT_HIGH
            {"type",     3},     // ATT_IMPORTANT
            {"duration", 10.0f},
        };
        TcpSession::DeliverPlayerAction(session_guid, action);
    }

    // Poll for READY, then PLAYER_REGISTER + GSC_GO_PLAY. Re-entrant per
    // session: TcpSession drops the request if that session already has a
    // travel poll running, so Use-spam can't stack timers.
    TcpSession::DeliverOpenWorldTravel(session_guid, row->map_name, kTaskForce);
}
