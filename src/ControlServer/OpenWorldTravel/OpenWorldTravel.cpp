#include "src/ControlServer/OpenWorldTravel/OpenWorldTravel.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/Database/Database.hpp"
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

    // Player-scoped alert on the player's CURRENT instance — they haven't gone
    // anywhere yet, so this lands where they're standing.
    void SendTravelAlert(const std::string& session_guid, const char* text, float duration) {
        nlohmann::json action;
        action["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
        action["session_guid"] = session_guid;
        action["action"]       = "alert_text";
        action["args"] = {
            {"text",     text},
            {"priority", 2},     // APT_HIGH
            {"type",     3},     // ATT_IMPORTANT
            {"duration", duration},
        };
        TcpSession::DeliverPlayerAction(session_guid, action);
    }

    // Shown when the volume names a destination we can't take the player to.
    // The open-world zones' own Map Transition volumes point at the quest
    // instances, which aren't built yet — those have no map_game_info row (or
    // one that isn't an open zone). Returning silently reads as a dead Use key,
    // so say what's actually going on.
    constexpr const char* kUnderConstruction = "Under construction, please come back later.";
    constexpr float       kUnderConstructionDuration = 6.0f;

    // Hidden one-map 'zone_*' queue for a sub-instance map
    // (night defense, Colony node/locker), seeded in Database::Init. 0 = none.
    uint32_t ZoneQueueForMap(const std::string& map_name) {
        sqlite3* db = Database::GetConnection();
        if (!db) return 0;
        uint32_t queue_id = 0;
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db,
                "SELECT q.queue_id FROM ga_queues q JOIN ga_map_pool_entries e"
                " ON e.map_pool_id = q.map_pool_id"
                " WHERE q.enabled = 0 AND q.name LIKE 'zone\\_%' ESCAPE '\\'"
                " AND e.map_name = ?1 AND e.enabled = 1 LIMIT 1", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(s, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(s) == SQLITE_ROW) queue_id = (uint32_t)sqlite3_column_int(s, 0);
            sqlite3_finalize(s);
        }
        return queue_id;
    }
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
        SendTravelAlert(session_guid, kUnderConstruction, kUnderConstructionDuration);
        return;
    }
    if (row->gameplay_type_value_id != MapGameInfo::kGameplayTypeOpenZone) {
        if (const uint32_t queue_id = ZoneQueueForMap(row->map_name)) {
            Logger::Log("travel",
                "[OpenWorldTravel] map_game_id=%u ('%s') is a zone sub-instance -> queue %u (guid=%s)\n",
                map_game_id, row->map_name.c_str(), queue_id, session_guid.c_str());
            TcpSession::DeliverZoneQueueJoin(session_guid, queue_id);
            return;
        }
        Logger::Log("travel",
            "[OpenWorldTravel] map_game_id=%u ('%s') gameplay_type=%u is not an open zone (%u) — refusing (guid=%s)\n",
            map_game_id, row->map_name.c_str(), row->gameplay_type_value_id,
            MapGameInfo::kGameplayTypeOpenZone, session_guid.c_str());
        SendTravelAlert(session_guid, kUnderConstruction, kUnderConstructionDuration);
        return;
    }
    if (row->map_name.empty() || row->game_class.empty()) {
        Logger::Log("travel",
            "[OpenWorldTravel] map_game_id=%u has an incomplete row (map_name='%s' game_class='%s')\n",
            map_game_id, row->map_name.c_str(), row->game_class.c_str());
        SendTravelAlert(session_guid, kUnderConstruction, kUnderConstructionDuration);
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
        SendTravelAlert(session_guid, "Instance is starting, please wait...", 10.0f);
    }

    // Poll for READY, then PLAYER_REGISTER + GSC_GO_PLAY. Re-entrant per
    // session: TcpSession drops the request if that session already has a
    // travel poll running, so Use-spam can't stack timers.
    // City maps (Dome) are everyone-friendly = task force 1, like the login/home path;
    // tf 2 there showed fellow players as enemies (red names).
    const int tf = (row->game_class == "TgGame.TgGame_City") ? 1 : kTaskForce;
    TcpSession::DeliverOpenWorldTravel(session_guid, row->map_name, tf);
}
