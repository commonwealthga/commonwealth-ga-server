#include "src/ControlServer/AvaParity/AvaParity.hpp"
#include "src/ControlServer/ExternalInstances/ExternalInstances.hpp"
#include "src/ControlServer/InstanceState/InstanceState.hpp"
#include "src/ControlServer/NodeClaims/NodeClaims.hpp"
#include "src/ControlServer/ApRewards/ApRewards.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/Shared/SourceRevision.hpp"
#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
#include "src/ControlServer/OpenWorldTravel/OpenWorldTravel.hpp"
#include "src/ControlServer/TcpListener/TcpListener.hpp"
#include "src/ControlServer/ChatListener/ChatListener.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include <filesystem>
#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/ControlServer/SpectatorOverlay/OverlayHttpServer.hpp"
#include "src/ControlServer/SpectatorOverlay/SpectatorOverlayState.hpp"
#include "src/ControlServer/SpectatorOverlay/MissionProgressState.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/MatchmakingService/SidePlacement.hpp"
#include "src/ControlServer/MmrService/MmrService.hpp"
#include <asio.hpp>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <csignal>
#include <functional>
#include <ctime>
#include <algorithm>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#if defined(__linux__)
#include <sched.h>
#include <unistd.h>
#include "src/ControlServer/BugReports/BugReports.hpp"
#include "src/ControlServer/MailService/MailService.hpp"
#include "src/ControlServer/AuctionService/AuctionService.hpp"
#include "src/ControlServer/MarketBots/MarketBots.hpp"
#include "src/ControlServer/AchievementService/AchievementService.hpp"
#include "src/ControlServer/EconParity/EconParity.hpp"
#include "src/ControlServer/Durability/Durability.hpp"
#include "src/ControlServer/ServiceAgents/ServiceAgents.hpp"
#include "src/ControlServer/MissionLoot/MissionLoot.hpp"
#endif

// ---------------------------------------------------------------------------
// Signal handling for clean shutdown
// ---------------------------------------------------------------------------

static asio::io_context* g_io = nullptr;

static void on_signal(int /*sig*/) {
    if (g_io) {
        g_io->stop();
    }
}

static bool UpdateQueueField(uint32_t queue_id, const std::string& field,
                              const nlohmann::json& value, std::string& message) {
    // Whitelist: each field has its own type + range gate. Anything not
    // listed here is rejected.
    enum class Kind { Bool, Uint, NonNegUint, NonNegFloat, Enum, DivisorList };
    struct FieldSpec { const char* name; Kind kind; };
    static const FieldSpec kSpecs[] = {
        { "enabled",                   Kind::Bool        },
        { "continue_in_queue",         Kind::Bool        },
        { "requires_pvp_verification", Kind::Bool        },
        { "instant_pop_when_full",     Kind::Bool        },
        { "min_players_to_pop",        Kind::Uint        },  // >= 1
        { "max_players_per_instance", Kind::NonNegUint  },  // >= 0
        { "max_players_per_side",     Kind::NonNegUint  },  // >= 0
        { "max_players_per_team",     Kind::NonNegUint  },  // >= 0
        { "max_team_size",            Kind::NonNegUint  },  // >= 0
        { "pop_delay_seconds",        Kind::NonNegFloat },  // >= 0
        { "taskforce_policy",         Kind::Enum        },
        { "team_policy",              Kind::Enum        },
        { "team_side_policy",         Kind::Enum        },
        { "pop_delay_policy",         Kind::Enum        },
        { "map_recency_divisors",     Kind::DivisorList },  // CSV, each >= 1; empty = off
        { "strict_class_balance",     Kind::Bool        },
        { "pair_backfill",            Kind::Bool        },
        { "setup_rebalance",          Kind::Bool        },
        { "late_join_policy",         Kind::Enum        },
        { "min_distinct_classes",     Kind::NonNegUint  },  // 0 = off
        { "max_class_per_side",       Kind::NonNegUint  },  // 0 = off
    };

    const FieldSpec* spec = nullptr;
    for (const auto& s : kSpecs) if (field == s.name) { spec = &s; break; }
    if (!spec) {
        message = "invalid queue field";
        return false;
    }

    int         bind_int    = 0;
    double      bind_double = 0.0;
    std::string bind_text;
    bool        is_double   = false;
    bool        is_text     = false;
    switch (spec->kind) {
        case Kind::Bool:
            if (!value.is_boolean()) { message = "value must be boolean"; return false; }
            bind_int = value.get<bool>() ? 1 : 0;
            break;
        case Kind::Uint: {
            if (!value.is_number_integer()) { message = "value must be integer"; return false; }
            int64_t v = value.get<int64_t>();
            if (v < 1) { message = std::string(spec->name) + " must be >= 1"; return false; }
            bind_int = (int)v;
            break;
        }
        case Kind::NonNegUint: {
            if (!value.is_number_integer()) { message = "value must be integer"; return false; }
            int64_t v = value.get<int64_t>();
            if (v < 0) { message = std::string(spec->name) + " must be >= 0"; return false; }
            bind_int = (int)v;
            break;
        }
        case Kind::NonNegFloat:
            if (!value.is_number()) { message = "value must be number"; return false; }
            bind_double = value.get<double>();
            if (bind_double < 0.0) { message = std::string(spec->name) + " must be >= 0"; return false; }
            is_double = true;
            break;
        case Kind::Enum: {
            if (!value.is_string()) { message = "value must be string"; return false; }
            bind_text = value.get<std::string>();
            // Validate with the same parsers the config loader uses, so the
            // accepted set can never drift from what ReloadQueues understands.
            bool ok = false;
            if      (field == "taskforce_policy") mm::ParseTaskforcePolicy(bind_text, &ok);
            else if (field == "team_policy")      mm::ParseTeamPolicy(bind_text, &ok);
            else if (field == "team_side_policy") mm::ParseTeamSidePolicy(bind_text, &ok);
            else if (field == "pop_delay_policy") mm::ParsePopDelayPolicy(bind_text, &ok);
            else if (field == "late_join_policy") mm::ParseLateJoinPolicy(bind_text, &ok);
            if (!ok) { message = "invalid value for " + field; return false; }
            is_text = true;
            break;
        }
        case Kind::DivisorList: {
            if (!value.is_string()) { message = "value must be string"; return false; }
            bind_text = value.get<std::string>();
            // Same parser the config loader uses — accepted syntax can't drift.
            bool ok = false;
            mm::ParseRecencyDivisors(bind_text, &ok);
            if (!ok) {
                message = "map_recency_divisors must be comma-separated numbers >= 1 (empty = off)";
                return false;
            }
            is_text = true;
            break;
        }
    }

    sqlite3* db = Database::GetConnection();
    if (!db) { message = "database is not open"; return false; }

    std::string sql = "UPDATE ga_queues SET " + field + " = ? WHERE queue_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        message = sqlite3_errmsg(db);
        return false;
    }
    if (is_text)        sqlite3_bind_text(stmt, 1, bind_text.c_str(), -1, SQLITE_TRANSIENT);
    else if (is_double) sqlite3_bind_double(stmt, 1, bind_double);
    else                sqlite3_bind_int(stmt, 1, bind_int);
    sqlite3_bind_int(stmt, 2, (int)queue_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) { message = sqlite3_errmsg(db); return false; }
    if (sqlite3_changes(db) == 0) { message = "queue not found"; return false; }

    if (is_text) {
        Logger::Log("matchmaking",
            "[Matchmaking] Admin update queue=%u field=%s value=%s → reloading\n",
            queue_id, field.c_str(), bind_text.c_str());
    } else if (is_double) {
        Logger::Log("matchmaking",
            "[Matchmaking] Admin update queue=%u field=%s value=%.2f → reloading\n",
            queue_id, field.c_str(), bind_double);
    } else {
        Logger::Log("matchmaking",
            "[Matchmaking] Admin update queue=%u field=%s value=%d → reloading\n",
            queue_id, field.c_str(), bind_int);
    }

    MatchmakingService::ReloadQueues();
    message = "queue updated and reloaded";
    return true;
}

static bool UpdatePoolEntry(uint32_t map_pool_id, const std::string& map_name,
                            const std::string& game_mode, const nlohmann::json& payload,
                            std::string& message) {
    if (map_name.empty() || game_mode.empty()) {
        message = "missing map_name or game_mode";
        return false;
    }

    sqlite3* db = Database::GetConnection();
    if (!db) {
        message = "database is not open";
        return false;
    }

    // Dynamic SET-clause builder: each entry is a SQL fragment + a bind
    // closure. Avoids the combinatorial explosion of optional-field
    // permutations. "= NULL" fragments use a no-op bind closure.
    struct SetClause {
        std::string sql_fragment;
        std::function<void(sqlite3_stmt*, int)> bind;
    };
    std::vector<SetClause> clauses;

    if (payload.contains("enabled") && payload["enabled"].is_boolean()) {
        const bool v = payload["enabled"].get<bool>();
        clauses.push_back({ "enabled = ?",
            [v](sqlite3_stmt* s, int i){ sqlite3_bind_int(s, i, v ? 1 : 0); } });
    }
    if (payload.contains("weight") && payload["weight"].is_number_integer()) {
        const int w = std::max(1, payload["weight"].get<int>());
        clauses.push_back({ "weight = ?",
            [w](sqlite3_stmt* s, int i){ sqlite3_bind_int(s, i, w); } });
    }
    // min_players / max_players: empty payload-value is impossible (caller
    // wouldn't include the key); null OR 0 → SQL NULL; positive integer
    // → that value; anything else rejected.
    for (const char* col : { "min_players", "max_players" }) {
        if (!payload.contains(col)) continue;
        const auto& v = payload[col];
        bool to_null = v.is_null() || (v.is_number_integer() && v.get<int>() == 0);
        if (to_null) {
            clauses.push_back({ std::string(col) + " = NULL",
                [](sqlite3_stmt*, int){} });
        } else if (v.is_number_integer() && v.get<int>() > 0) {
            const int n = v.get<int>();
            clauses.push_back({ std::string(col) + " = ?",
                [n](sqlite3_stmt* s, int i){ sqlite3_bind_int(s, i, n); } });
        } else {
            message = std::string(col) + " must be a non-negative integer or null";
            return false;
        }
    }

    if (clauses.empty()) {
        message = "no editable fields in payload";
        return false;
    }

    std::string sql = "UPDATE ga_map_pool_entries SET ";
    for (size_t i = 0; i < clauses.size(); ++i) {
        if (i > 0) sql += ", ";
        sql += clauses[i].sql_fragment;
    }
    sql += " WHERE map_pool_id = ? AND map_name = ? AND game_mode = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        message = sqlite3_errmsg(db);
        return false;
    }

    int col = 1;
    for (const auto& c : clauses) {
        c.bind(stmt, col);  // no-op for "= NULL" fragments
        if (c.sql_fragment.find('?') != std::string::npos) col++;
    }
    sqlite3_bind_int(stmt, col++, (int)map_pool_id);
    sqlite3_bind_text(stmt, col++, map_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, col++, game_mode.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) { message = sqlite3_errmsg(db); return false; }
    if (sqlite3_changes(db) == 0) { message = "pool entry not found"; return false; }

    // Summary log: what was set, comma-joined.
    std::string summary;
    if (payload.contains("enabled") && payload["enabled"].is_boolean()) {
        if (!summary.empty()) summary += ",";
        summary += std::string("enabled=") + (payload["enabled"].get<bool>() ? "1" : "0");
    }
    if (payload.contains("weight") && payload["weight"].is_number_integer()) {
        if (!summary.empty()) summary += ",";
        summary += "weight=" + std::to_string(std::max(1, payload["weight"].get<int>()));
    }
    for (const char* k : { "min_players", "max_players" }) {
        if (!payload.contains(k)) continue;
        const auto& v = payload[k];
        if (!summary.empty()) summary += ",";
        summary += std::string(k) + "=";
        if (v.is_null() || (v.is_number_integer() && v.get<int>() == 0)) {
            summary += "NULL";
        } else {
            summary += std::to_string(v.get<int>());
        }
    }
    Logger::Log("matchmaking",
        "[Matchmaking] Admin update pool_entry=%u:%s:%s set=%s → reloading\n",
        map_pool_id, map_name.c_str(), game_mode.c_str(), summary.c_str());

    MatchmakingService::ReloadQueues();
    message = "pool entry updated and queues reloaded";
    return true;
}

static bool SpawnAdminInstance(const ControlServerConfig& cfg, const std::string& map_name,
                               const std::string& game_mode, int max_players,
                               uint32_t queue_id, std::string& message,
                               const nlohmann::json& group = nlohmann::json::array(),
                               const nlohmann::json& context = nlohmann::json::object()) {
    if (map_name.empty() || game_mode.empty()) {
        message = "missing map_name or game_mode";
        return false;
    }
    if (max_players <= 0 || max_players > 1000) {
        message = "max_players must be between 1 and 1000";
        return false;
    }

    auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
    if (!port) {
        message = "no UDP ports available";
        return false;
    }

    // Optional queue_id (dev/coverage tests): run at that queue's difficulty
    // and tag the instance with it, like a matchmade spawn (rewards, loot,
    // achievements key off the queue). 0 = plain ad-hoc instance.
    uint32_t difficulty = 0;
    if (queue_id != 0) {
        auto qcfg = MatchmakingService::GetQueueConfig(queue_id);
        if (!qcfg) {
            message = "unknown queue_id";
            return false;
        }
        difficulty = qcfg->difficulty_value_id;
    }

    int64_t instance_id = InstanceRegistry::InsertStarting(
        map_name, game_mode, *port, 0, /*is_home_map=*/false, queue_id);
    InstanceRegistry::SetMaxPlayers(instance_id, max_players);

    // Optional "group" [{user_id, character_id}]: offer the instance to the
    // external_instance_group service first (same path as a matchmade group
    // pop); "context" is passed through to the service. Refused -> local.
    if (group.is_array() && !group.empty()) {
        std::vector<ExternalInstances::Member> members;
        for (const auto& g : group) {
            if (!g.is_object()) { members.clear(); break; }
            members.push_back({g.value("user_id", (int64_t)0), g.value("character_id", (int64_t)0)});
        }
        nlohmann::json extra = context.is_object() ? context : nlohmann::json::object();
        extra["queue_id"] = queue_id;
        extra["map_name"] = map_name;
        extra["game_mode"] = game_mode;
        extra["udp_port"] = *port;
        extra["difficulty"] = difficulty;
        std::string address;
        if (!members.empty() && ExternalInstances::AttachGroup(instance_id, members, extra, &address)) {
            InstanceRegistry::SetAddress(instance_id, address);
            ExternalInstances::ArmReadyTimer(instance_id, cfg.external_ready_timeout_s);
            Logger::Log("main", "Admin action: created instance %lld externally at %s port=%d map=%s mode=%s players=%d queue=%u\n",
                (long long)instance_id, address.c_str(), (int)*port, map_name.c_str(), game_mode.c_str(),
                (int)members.size(), queue_id);
            message = "instance created: " + std::to_string(instance_id) + " external";
            return true;
        }
    }

    pid_t pid = InstanceSpawner::Spawn(cfg, map_name, game_mode, *port, instance_id, eDifficulty::Id, difficulty);
    if (pid < 0) {
        InstanceRegistry::MarkStopped(instance_id);
        message = "InstanceSpawner::Spawn failed";
        return false;
    }

    InstanceRegistry::UpdatePid(instance_id, pid);
    Logger::Log("main", "Admin action: created instance %lld pid=%d port=%d map=%s mode=%s max_players=%d queue=%u difficulty=%u\n",
        (long long)instance_id, (int)pid, (int)*port, map_name.c_str(), game_mode.c_str(),
        max_players, queue_id, difficulty);
    message = "instance created: " + std::to_string(instance_id);
    return true;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::string config_path = "control-server.json";
    bool wine_debug = false;

    // Parse CLI arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            fprintf(stdout,
                "Usage: control-server [OPTIONS]\n"
                "\n"
                "Options:\n"
                "  --config=PATH     Path to JSON config file (default: control-server.json)\n"
                "  --config PATH     (space-separated form)\n"
                "  --wine-debug      Use winedbg with --command cont (prints crash traces)\n"
                "  --help, -h        Print this help and exit\n"
            );
            return 0;
        }
        else if (arg == "--wine-debug") {
            wine_debug = true;
        }
        else if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
        else if (arg.rfind("--config=", 0) == 0) {
            config_path = arg.substr(9);
        }
        else {
            fprintf(stderr, "[control-server] Unknown argument: %s (use --help for usage)\n",
                arg.c_str());
        }
    }

    // Load config from JSON file (falls back to defaults if file is absent).
    ControlServerConfig cfg = ControlServerConfig::Load(config_path);
    cfg.wine_debug = wine_debug;

#if defined(__linux__)
    // Exclude the control server's own PID from game_cpu_range. Spawned
    // instances are pinned into that range via sched_setaffinity, but
    // nothing previously stopped the kernel from also scheduling the
    // control server (matchmaking loop, DB writes, IPC, HTTP overlay)
    // onto those same cores, causing contention with live matches.
    if (cfg.game_cpu_relative) {
        cpu_set_t allowed;
        CPU_ZERO(&allowed);
        if (sched_getaffinity(0, sizeof(allowed), &allowed) == 0) {
            for (int c = 0; c < CPU_SETSIZE; ++c) if (CPU_ISSET(c, &allowed)) cfg.cpu_map.push_back(c);
        }
        std::string list;
        for (int c : cfg.cpu_map) list += (list.empty() ? "" : ",") + std::to_string(c);
        Logger::Log("main", "game_cpu_relative: allowed cpus %s\n", list.c_str());
    }
    if (cfg.game_cpu_relative && cfg.game_cpu_range.lo >= 0 && cfg.game_cpu_range.hi >= cfg.game_cpu_range.lo) {
        // Keep the control server off the game cpus (positions outside the range).
        cpu_set_t mask;
        CPU_ZERO(&mask);
        for (int i = 0; i < (int)cfg.cpu_map.size(); ++i) {
            if (i < cfg.game_cpu_range.lo || i > cfg.game_cpu_range.hi) CPU_SET(cfg.cpu_map[i], &mask);
        }
        if (CPU_COUNT(&mask) > 0) sched_setaffinity(0, sizeof(mask), &mask);
        else Logger::Log("main", "game_cpu_range covers all cores; control server left unpinned\n");
    } else if (cfg.game_cpu_range.lo >= 0 && cfg.game_cpu_range.hi >= cfg.game_cpu_range.lo) {
        const long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
        if (nprocs > 0) {
            cpu_set_t mask;
            CPU_ZERO(&mask);
            for (long c = 0; c < nprocs; ++c) {
                if (c < cfg.game_cpu_range.lo || c > cfg.game_cpu_range.hi) {
                    CPU_SET(static_cast<int>(c), &mask);
                }
            }
            if (CPU_COUNT(&mask) > 0) {
                sched_setaffinity(0, sizeof(mask), &mask);
            } else {
                Logger::Log("main", "game_cpu_range covers all cores; control server left unpinned\n");
            }
        }
    }
#endif

    // Signal handlers for clean shutdown
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);
#ifndef _WIN32
    std::signal(SIGCHLD, SIG_IGN);  // Auto-reap child zombies (Linux)
#endif

    Logger::Log("main", "Control server v0.0.9 starting...\n");

    // Initialize database
    Database::SetDbPath(cfg.db_path);
    Database::Init();
    if (!AvaParity::MigrateZoneLayout()) return 3;  // War Zone 12x30 (one-time clear; refuses to run over player holdings)
    MailService::EnsureSchema();  // ga_mail
    BugReports::EnsureSchema();   // ga_bug_reports (-bug / -report)
    AuctionService::EnsureSchema();  // ga_auctions
    MarketBots::EnsureSchema();      // ga_market_bots + log
    MarketBots::Configure(cfg.market_bots);
    ApRewards::EnsureSchema();       // ga_ap_grants (AP audit + daily cap)
    ApRewards::Configure(cfg.ap_rewards);
    ServiceHooks::Configure(cfg.services);
    AchievementService::EnsureSchema();  // ga_character_achievements
    MissionLoot::EnsureSchema();  // ga_pending_mission_loot
    EconParity::InitSchema();  // lv50 xp -> credits trigger
    Durability::EnsureSchema();  // ga_players_inventory.durability (-1 = full)

    // Preload the map_game_info registry from the shared DB. Safe to call
    // even if the table doesn't exist yet (control server may start before
    // any game-server has applied schema v51) — lookups just miss and
    // send_go_play_to_instance falls back to its hardcoded HQ defaults.
    MapGameInfo::Init();

    // Initialize session store
    PlayerSessionStore::Init();
    PlayerSessionStore::SetStarterKit(cfg.starter_kit_only, cfg.starter_kit_max_level);
    PlayerSessionStore::SetTokenLimits(cfg.token_weekly_earn, cfg.token_bank);
    Database::SetChoiceLootTables(cfg.quest_reward_choice_tables);

    // Initialize instance registry
    InstanceRegistry::Init();
    //InstanceRegistry::SetHost(cfg.host); //skal: unused

    // Create ASIO io_context and register signal handler reference
    // skal - moved here cause it's required during TcpSession::Init() if there is asio stuff done there
    //        such as dns resolution and the like - this was the source of my crashes
    asio::io_context io;
    g_io = &io;

    // Set network config for TcpSession responses (external IP + chat port)
    TcpSession::SetHomeMapName(cfg.home_map_name);
    TcpSession::SetHomeArrivalMinLevel(cfg.home_arrival_min_level);
    if (!TcpSession::Init(io, cfg)) return 1;
    InstanceRegistry::SetMaxInstances(cfg.max_instances);
    TcpSession::SetMaxOnlinePlayers(cfg.max_online_players);
    TcpSession::SetZoneCap(cfg.zone_cap);
    EventCalendar::Set(cfg.calendar);
    {
        std::string on;
        for (const auto& k : EventCalendar::ActiveKeys()) on += (on.empty() ? "" : ",") + k;
        Logger::Log("main", "Calendar: %zu windows, open now: [%s]\n", cfg.calendar.size(), on.c_str());
    }
    Logger::Log("main", "Limited launch caps: max_online_players=%d max_instances=%d dome_cap=%d zone_cap=%d\n",
        cfg.max_online_players, cfg.max_instances, cfg.dome_cap, cfg.zone_cap);
    // Set IPC server stats flags
    IpcServer::SetStatsToggles(cfg.device_stats_enabled, cfg.effectiveness_enabled);

    // Clear stale instances from any previous (crashed) run
    InstanceRegistry::ClearStaleInstances();

    // Provision per-slot WINEPREFIXes if enabled. Idempotent — only clones
    // missing prefixes. Must run before any Spawn() so the first match
    // doesn't pay clone latency.
    if (!InstanceSpawner::EnsureSlotPrefixes(cfg)) {
        Logger::Log("main", "EnsureSlotPrefixes failed — aborting startup\n");
        return 1;
    }

    // Initialize matchmaking. Init() reads ga_queues + ga_map_pool_entries from
    // the shared DB and constructs one MatchmakingService::Queue per enabled
    // row via RuleFactory. Edit queues without rebuilding by changing those
    // tables — see Database.cpp migrations + the `-reload-queues` chat command
    // for hot-reload semantics.
    //
    // Map pool note: maps that need to be downloaded by the client to work
    // (e.g. 3P_Beachhead2_P, 3P_Beachhead_P) belong in ga_map_pool_entries with
    // enabled=0 until the client downloads them.
    MatchmakingService::Init();

    // Seed / catch up the MMR engines (design 2026-07-12). First boot on an
    // empty history replays the entire match backlog; later boots fold
    // anything concluded while the control server was down.
    MmrService::FoldUnprocessed();

    // Provide queue-scoped running-instance info to matchmaking rules. Rules
    // iterate this list to find a seat-available instance regardless of map —
    // because pool-randomized queues can have instances on different maps,
    // map equality is no longer the join criterion. Filter is done in-memory
    // (small N — only READY mission instances live).
    MatchmakingService::SetInstanceProvider([](uint32_t queue_id) -> std::vector<RunningInstance> {
        // Gather ALL live state here so the rules stay pure functions of
        // (parties, instances): per-team seeds (live roster + reserved-but-not-
        // yet-arrived), access mode + owner parties, seats.
        auto all = InstanceRegistry::GetReadyMissionInstances();
        auto qcfg = MatchmakingService::GetQueueConfig(queue_id);
        const int queue_cap = qcfg ? (int)MatchmakingService::GetQueueInstanceCap(*qcfg) : 0;

        auto parse_owner_csv = [](const std::string& csv) {
            std::vector<uint64_t> ids;
            size_t i = 0;
            while (i < csv.size()) {
                size_t j = csv.find(',', i);
                if (j == std::string::npos) j = csv.size();
                if (j > i) ids.push_back(strtoull(csv.substr(i, j - i).c_str(), nullptr, 10));
                i = j + 1;
            }
            return ids;
        };

        std::vector<RunningInstance> filtered;
        filtered.reserve(all.size());
        for (const auto& inst : all) {
            if (inst.queue_id != queue_id) continue;
            RunningInstance ri;
            ri.instance_id  = inst.instance_id;
            ri.map_name     = inst.map_name;
            ri.game_mode    = inst.game_mode;
            ri.max_players  = queue_cap > 0 ? queue_cap : inst.max_players;
            ri.queue_id     = inst.queue_id;
            ri.access_mode  = mm::ParseAccessMode(inst.access_mode);
            ri.owner_party_ids = parse_owner_csv(inst.owner_party_ids);
            ri.started_at   = inst.started_at;   // late-backfill gate
            // Live roster -> per-team seed. MMR sums only for BalancedPvp
            // queues (steers newcomers toward the weaker side; neutral
            // elsewhere so non-PvP placement is unchanged).
            const auto seed_qcfg = MatchmakingService::GetQueueConfig(inst.queue_id);
            const bool seed_mmr_aware = seed_qcfg
                && seed_qcfg->taskforce_policy == TaskforcePolicy::BalancedPvp;
            for (const auto& r : InstanceRegistry::GetActivePlayersForInstance(inst.instance_id)) {
                TeamSeed& seed = (r.task_force == 2) ? ri.team2 : ri.team1;
                seed.size += 1;
                seed.heal_score += SidePlacement::HealValue(r.profile_id, r.task_force == 2 ? 2 : 1);
                seed.class_counts[r.profile_id] += 1;
                if (seed_mmr_aware) {
                    const double mmr =
                        MmrService::GetCurrentRating(r.user_id, r.profile_id);
                    seed.mmr_sum += mmr;
                    seed.class_mmr_sum[r.profile_id] += mmr;
                }
            }
            ri.player_count = ri.team1.size + ri.team2.size;
            filtered.push_back(std::move(ri));
        }

        // Merge reserved (popped-but-not-yet-registered) contributions.
        for (auto& reserved : MatchmakingService::GetReservedReadyInstances(queue_id)) {
            auto it = std::find_if(filtered.begin(), filtered.end(),
                [&](const RunningInstance& ri) { return ri.instance_id == reserved.instance_id; });
            if (it == filtered.end()) {
                reserved.max_players = queue_cap > 0 ? queue_cap : reserved.max_players;
                filtered.push_back(std::move(reserved));
                continue;
            }
            auto merge = [](TeamSeed& dst, const TeamSeed& src) {
                dst.size += src.size;
                dst.heal_score += src.heal_score;
                dst.mmr_sum += src.mmr_sum;
                for (const auto& [cls, n] : src.class_counts) dst.class_counts[cls] += n;
                for (const auto& [cls, v] : src.class_mmr_sum) dst.class_mmr_sum[cls] += v;
            };
            merge(it->team1, reserved.team1);
            merge(it->team2, reserved.team2);
            it->player_count = it->team1.size + it->team2.size;
        }

        filtered.erase(
            std::remove_if(filtered.begin(), filtered.end(),
                [](const RunningInstance& ri) {
                    return ri.max_players > 0 && ri.player_count >= ri.max_players;
                }),
            filtered.end());
        return filtered;
    });

    // Handle match pop: spawn instance or route to existing one
    MatchmakingService::SetMatchPopCallback(
        [&cfg](uint32_t queue_id, MatchResult result) {
            if (result.existing_instance_id) {
                // Route to existing instance — send invitation immediately
                Logger::Log("matchmaking",
                    "[Matchmaking] Routing %zu players to existing instance %lld\n",
                    result.session_guids.size(), (long long)*result.existing_instance_id);
                uint32_t cap = 0;
                if (auto qcfg = MatchmakingService::GetQueueConfig(queue_id)) {
                    cap = result.cap_override.value_or(
                        MatchmakingService::GetQueueInstanceCap(*qcfg));
                }
                MatchmakingService::TrackReadyMatchReservations(
                    *result.existing_instance_id, queue_id, result.game_mode,
                    result.session_guids, result.task_force_assignments,
                    result.profile_ids, result.mmrs, cap);
                for (const auto& guid : result.session_guids) {
                    int tf = 1;
                    auto it = result.task_force_assignments.find(guid);
                    if (it != result.task_force_assignments.end()) tf = it->second;
                    TcpSession::DeliverMatchInvitation(guid, *result.existing_instance_id, result.game_mode, tf);
                }
                return;
            }

            // Spawn new instance
            // A one-player match that popped without a free local slot
            // (external probe said yes) must run externally; a loopback-hosted
            // instance uses no port of our pool, so any port number will do.
            const bool external_only = result.session_guids.size() == 1 &&
                MatchmakingService::TakeExternalOnly(result.session_guids[0]);
            auto port = InstanceRegistry::AllocatePort(
                cfg.udp_port_range.lo, cfg.udp_port_range.hi);
            // one player and no free pool port: try the external host first on a
            // borrowed (loopback) port; local spawn is then impossible.
            const bool port_borrowed = !port && result.session_guids.size() == 1;
            if (port_borrowed) port = cfg.udp_port_range.lo;
            if (!port) {
                Logger::Log("matchmaking",
                    "[Matchmaking] No UDP ports available for match spawn - %zu player(s) told\n",
                    result.session_guids.size());
                for (const auto& g : result.session_guids) {
                    TcpSession::DeliverMatchCancelled(g, "no free server port");
                    ChatSession::SystemMessageToGuid(g,
                        "All mission servers are busy right now - please queue again in a minute.");
                }
                return;
            }

            // Carry the rule's access semantics onto the instance row so the
            // provider (and the invite-into-locked-match path) can read them
            // back. owner_party_ids -> CSV.
            std::string owner_csv;
            for (size_t i = 0; i < result.owner_party_ids.size(); ++i) {
                if (i) owner_csv += ",";
                owner_csv += std::to_string(result.owner_party_ids[i]);
            }
            const std::string access_str = mm::AccessModeToString(result.access_mode);

            int64_t instance_id = InstanceRegistry::InsertStarting(
                result.map_name, result.game_mode, *port, 0, /*is_home_map=*/false,
                /*queue_id=*/queue_id, /*predecessor_instance_id=*/0,
                access_str, owner_csv);

            // Register pending match BEFORE Spawn so the unwind path on a
            // spawn-failure (and the symmetric IPC-disconnect-before-READY
            // path in IpcServer) can call DiscardPendingMatchForDeadInstance
            // to unstick the matched players. Without this, a process that
            // fails to fork leaves its session_guids out of queue.players
            // with no way back.
            PendingMatch pending;
            pending.instance_id = instance_id;
            pending.queue_id = queue_id;
            pending.map_name = result.map_name;
            pending.game_mode = result.game_mode;
            pending.access_mode = result.access_mode;
            pending.owner_party_ids = result.owner_party_ids;
            pending.session_guids = std::move(result.session_guids);
            pending.task_force_assignments = std::move(result.task_force_assignments);
            pending.profile_ids = std::move(result.profile_ids);
            pending.mmrs = std::move(result.mmrs);
            // Honour rule-supplied cap_override (e.g. DoubleAgentRule seals
            // each match at its roster size). Falls back to the queue-wide
            // cap when the rule didn't override.
            auto qcfg = MatchmakingService::GetQueueConfig(queue_id);
            if (qcfg) {
                pending.cap = result.cap_override.value_or(
                    MatchmakingService::GetQueueInstanceCap(*qcfg));
            }
            const std::string only_guid =
                pending.session_guids.size() == 1 ? pending.session_guids[0] : std::string();
            const std::vector<std::string> match_guids = pending.session_guids;
            MatchmakingService::AddPendingMatch(instance_id, std::move(pending));

            // Look up queue's configured difficulty so the instance runs
            // at the queue's tier instead of the DLL's map-name heuristic.

            // skal: intercept that and look for overriden difficulty (wich comes through the MatchResult)
            eDifficulty difficulty_type = eDifficulty::Default;
            uint32_t difficulty_id = 0;
            DifficultyScalar difficulty_scalar;
            if (result.difficulty_override) {
                difficulty_scalar = result.difficulty_override;
                difficulty_type = eDifficulty::Scalar;
                Logger::Log("main","[main] spawn instance, will use overriden difficulty: %.2f/%.2f\n",difficulty_scalar.HP,difficulty_scalar.Dmg);
            } else if (qcfg) {
                difficulty_id = qcfg->difficulty_value_id;
                if (difficulty_id != 0) {
                    difficulty_type = eDifficulty::Id;
                    Logger::Log("main","[main] spawn instance, will use difficulty %d\n",difficulty_id);
                } else {
                    Logger::Log("main","[main] spawn instance, will use default difficulty (from queue config)\n");
                }
            } else {
                Logger::Log("main","[main] spawn instance, will use default difficulty\n");
            }

            // One-player match: offer it to the external instance service
            // first. Accepted -> no local process; clients go to the reply's
            // address; not READY in time -> started here (ExternalInstances).
            if (!only_guid.empty()) {
                auto sess = PlayerSessionStore::GetByGuid(only_guid);
                std::string address;
                if (sess && ExternalInstances::Attach(sess->user_id, instance_id,
                        sess->selected_character_id,
                        {{"queue_id", queue_id}, {"map_name", result.map_name},
                         {"game_mode", result.game_mode}, {"udp_port", *port},
                         {"difficulty", queue_difficulty}, {"players", 1}},
                        &address)) {
                    InstanceRegistry::SetAddress(instance_id, address);
                    ExternalInstances::ArmReadyTimer(instance_id, cfg.external_ready_timeout_s);
                    Logger::Log("matchmaking",
                        "[Matchmaking] Instance %lld for queue %u runs externally at %s:%d\n",
                        (long long)instance_id, queue_id, address.c_str(), (int)*port);
                    return;
                }
                if (external_only || port_borrowed) {  // no local slot/port for it - never start it here
                    Logger::Log("matchmaking",
                        "[Matchmaking] Instance %lld: external host refused after probe, no local slot - cancelled\n",
                        (long long)instance_id);
                    MatchmakingService::DiscardPendingMatchForDeadInstance(instance_id, "external host unavailable");
                    InstanceRegistry::MarkStopped(instance_id);
                    ChatSession::SystemMessageToGuid(only_guid,
                        "Your PC could not start the mission and all mission servers are busy - please queue again.");
                    return;
                }
            }

            // Several players: same offer to the group service, which names
            // the hosting player. Refused / no service -> local as before.
            if (match_guids.size() > 1) {
                std::vector<ExternalInstances::Member> members;
                for (const auto& g : match_guids) {
                    auto sess = PlayerSessionStore::GetByGuid(g);
                    if (!sess) { members.clear(); break; }
                    members.push_back({sess->user_id, sess->selected_character_id});
                }
                std::string address;
                if (!members.empty() && ExternalInstances::AttachGroup(instance_id, members,
                        {{"queue_id", queue_id}, {"map_name", result.map_name},
                         {"game_mode", result.game_mode}, {"udp_port", *port},
                         {"difficulty", queue_difficulty}},
                        &address)) {
                    InstanceRegistry::SetAddress(instance_id, address);
                    ExternalInstances::ArmReadyTimer(instance_id, cfg.external_ready_timeout_s);
                    Logger::Log("matchmaking",
                        "[Matchmaking] Instance %lld for queue %u (%d players) runs externally at %s:%d\n",
                        (long long)instance_id, queue_id, (int)members.size(), address.c_str(), (int)*port);
                    return;
                }
            }

            pid_t pid = InstanceSpawner::Spawn(
                cfg, result.map_name, result.game_mode, *port, instance_id, difficulty_type, difficulty_id, difficulty_scalar);

            if (pid < 0) {
                Logger::Log("matchmaking",
                    "[Matchmaking] Failed to spawn instance for queue %u\n", queue_id);
                MatchmakingService::DiscardPendingMatchForDeadInstance(
                    instance_id, "InstanceSpawner::Spawn returned -1");
                InstanceRegistry::MarkStopped(instance_id);
                return;
            }

            InstanceRegistry::UpdatePid(instance_id, pid);

            Logger::Log("matchmaking",
                "[Matchmaking] Spawned instance %lld (pid=%d port=%d) for queue %u\n",
                (long long)instance_id, (int)pid, (int)*port, queue_id);
        }
    );

    // Successor spawner — invoked from IpcServer's MSG_REQUEST_SUCCESSOR
    // handler after dedupe. Picks a fresh (map, game_mode) at random from
    // the parent's queue's map_pool, so successors don't necessarily
    // inherit the parent's map — that's the whole point of pool-based
    // randomization. Binds the new row via predecessor_instance_id so
    // MarkReady lands it in DRAFTING (not READY); MSG_MISSION_ENDED on the
    // parent promotes DRAFTING → READY.
    // External instance given up before READY: run it here on the same row
    // and port; the pending match is invited when this process is READY.
    ExternalInstances::SetFallback([&cfg](int64_t instance_id) -> bool {
        auto inst = InstanceRegistry::GetInstanceById(instance_id);
        if (!inst || inst->state != "STARTING") return false;
        if (!InstanceRegistry::HasFreeMissionSlot()) {
            Logger::Log("main", "[ExternalFallback] instance cap reached - cannot start %lld here\n",
                (long long)instance_id);
            return false;
        }
        uint32_t difficulty = 0;
        if (inst->queue_id != 0) {
            if (auto qcfg = MatchmakingService::GetQueueConfig(inst->queue_id))
                difficulty = qcfg->difficulty_value_id;
        }
        // The row may carry a loopback port that a local instance already
        // uses; take a real free port of the pool (this row is still 127.0.0.1,
        // so AllocatePort does not count it).
        auto real_port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
        if (!real_port) {
            Logger::Log("main", "[ExternalFallback] no free UDP port - cannot start %lld here\n",
                (long long)instance_id);
            return false;
        }
        InstanceRegistry::SetPort(instance_id, *real_port);
        inst->udp_port = *real_port;
        InstanceRegistry::SetAddress(instance_id, "");
        pid_t pid = InstanceSpawner::Spawn(cfg, inst->map_name, inst->game_mode,
                                           inst->udp_port, instance_id, eDifficulty::Id, difficulty);
        if (pid < 0) return false;
        InstanceRegistry::UpdatePid(instance_id, pid);
        Logger::Log("main", "[ExternalFallback] instance %lld started here pid=%d port=%d map=%s\n",
            (long long)instance_id, (int)pid, (int)inst->udp_port, inst->map_name.c_str());
        return true;
    });

    IpcServer::SetSuccessorSpawner([&cfg](int64_t parent_instance_id) {
        auto parent = InstanceRegistry::GetInstanceById(parent_instance_id);
        if (!parent) {
            Logger::Log("main", "[SuccessorSpawner] parent_instance_id=%lld not found — aborting\n",
                (long long)parent_instance_id);
            return;
        }
        if (parent->queue_id == 0) {
            Logger::Log("main", "[SuccessorSpawner] parent_instance_id=%lld has no queue_id — refusing to spawn (continuation requires a queue context)\n",
                (long long)parent_instance_id);
            return;
        }
        // The DLL fires MSG_REQUEST_SUCCESSOR from any mission whose mode/score
        // condition trips — it doesn't know whether its queue is opted into
        // continuation. Gate here so non-continuing queues (e.g. SpecOps)
        // don't burn a slot on a successor nobody will ever join.
        if (!MatchmakingService::GetContinueInQueue(parent->queue_id)) {
            Logger::Log("main", "[SuccessorSpawner] Queue %u is not continue_in_queue — dropping successor request for parent=%lld\n",
                parent->queue_id, (long long)parent_instance_id);
            return;
        }
        const auto active = InstanceRegistry::GetActivePlayersForInstance(parent_instance_id);
        const int parent_count = (int)active.size();
        // DLC-locked pools: the successor carries the parent's players, so
        // narrow the pick to maps every continuing player owns (else most-owned).
        std::vector<std::vector<int64_t>> installed_sets;
        if (auto qcfg = MatchmakingService::GetQueueConfig(parent->queue_id)) {
            if (mm::PoolHasDlcMaps(*qcfg))
                for (const auto& ap : active)
                    installed_sets.push_back(Database::GetInstalledDlcIds(ap.user_id));
        }
        auto picked = MatchmakingService::PickRandomMapPoolEntryForCount(
            parent->queue_id, parent_count,
            installed_sets.empty() ? nullptr : &installed_sets);
        if (!picked) {
            Logger::Log("main", "[SuccessorSpawner] Queue %u has no map_pool configured — refusing successor for parent=%lld\n",
                parent->queue_id, (long long)parent_instance_id);
            return;
        }
        if (!InstanceRegistry::HasFreeMissionSlot()) {
            Logger::Log("main", "[SuccessorSpawner] instance cap reached - no successor for parent=%lld\n",
                (long long)parent_instance_id);
            return;
        }
        auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
        if (!port) {
            Logger::Log("main", "[SuccessorSpawner] No UDP ports available for successor of %lld\n",
                (long long)parent_instance_id);
            return;
        }
        int64_t instance_id = InstanceRegistry::InsertStarting(
            picked->map_name, picked->game_mode, *port, 0, /*is_home_map=*/false,
            /*queue_id=*/parent->queue_id,
            /*predecessor_instance_id=*/parent_instance_id);
        // Successor inherits parent queue's difficulty_value_id.
        uint32_t queue_difficulty = 0;
        if (auto qcfg = MatchmakingService::GetQueueConfig(parent->queue_id)) {
            queue_difficulty = qcfg->difficulty_value_id;
        }
        // skal: afaik this is never used for pve mission, therefore the difficulty is always stock one
        //Logger::Log("skal","[main] spawn instance, default difficulty from being 'successor' map)\n");
        pid_t pid = InstanceSpawner::Spawn(
            cfg, picked->map_name, picked->game_mode, *port, instance_id, eDifficulty::Id, queue_difficulty);
        if (pid < 0) {
            Logger::Log("main", "[SuccessorSpawner] Failed to spawn successor for parent=%lld\n",
                (long long)parent_instance_id);
            InstanceRegistry::MarkStopped(instance_id);
            return;
        }
        InstanceRegistry::UpdatePid(instance_id, pid);
        Logger::Log("main", "[SuccessorSpawner] Spawned successor %lld (pid=%d port=%d map=%s mode=%s) for parent=%lld queue=%u — will land in DRAFTING on INSTANCE_READY\n",
            (long long)instance_id, (int)pid, (int)*port,
            picked->map_name.c_str(), picked->game_mode.c_str(),
            (long long)parent_instance_id, parent->queue_id);
    });

    // Open-world zone spawner. Called by OpenWorldTravel when a Map Transition
    // omega volume targets a zone with no live instance. The one-instance-per-
    // map guarantee lives in OpenWorldTravel — this just does the spawn.
    // queue_id stays 0: these maps are never matchmade.
    OpenWorldTravel::SetSpawner([&cfg](const std::string& map_name,
                                       const std::string& game_mode,
                                       uint32_t difficulty_value_id) -> int64_t {
        if (!InstanceRegistry::HasFreeMissionSlot()) {
            Logger::Log("travel", "[OpenWorldSpawner] instance cap reached - not spawning '%s'\n",
                map_name.c_str());
            return 0;
        }
        auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
        if (!port) {
            Logger::Log("travel", "[OpenWorldSpawner] No UDP ports available for '%s'\n",
                map_name.c_str());
            return 0;
        }

        int64_t instance_id = InstanceRegistry::InsertStarting(
            map_name, game_mode, *port, 0, /*is_home_map=*/false);
        Logger::Log("main","[main] spawn instance, will use default difficulty (from being open world map)\n");
        pid_t pid = InstanceSpawner::Spawn(cfg, map_name, game_mode, *port, instance_id,
                                           eDifficulty::Id, difficulty_value_id, DifficultyScalar(), cfg.zone_cap);
        if (pid < 0) {
            Logger::Log("travel", "[OpenWorldSpawner] Spawn failed for '%s'\n", map_name.c_str());
            InstanceRegistry::MarkStopped(instance_id);
            return 0;
        }

        InstanceRegistry::UpdatePid(instance_id, pid);
        Logger::Log("travel",
            "[OpenWorldSpawner] Spawned '%s' instance_id=%lld pid=%d port=%d difficulty=%u\n",
            map_name.c_str(), (long long)instance_id, (int)pid, (int)*port, difficulty_value_id);
        return instance_id;
    });

    // Solo intro/training mission for freshly created agents.
    // One instance per new agent (not shared); idle cleanup reaps it after use.
    if (cfg.intro_mission_enabled) {
        // Tutorial chapters
        TcpSession::SetIntroChain(cfg.intro_mission_map_name, cfg.intro_chain_maps, cfg.intro_chain_user_ids);
        // Start an intro row here (slot + real port); the row counts itself.
        auto intro_start_here = [&cfg](int64_t instance_id, const char* why) -> bool {
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state != "STARTING") return false;
            const int max = InstanceRegistry::MaxInstances();
            if (max > 0 && InstanceRegistry::CountActiveNonHomeInstances() - ExternalInstances::Count() - 1 >= max - 1) {
                Logger::Log("main", "[IntroSpawner] instance cap reached - '%s' (%lld) cannot start here (%s)\n",
                    inst->map_name.c_str(), (long long)instance_id, why);
                return false;
            }
            InstanceRegistry::SetAddress(instance_id, "127.0.0.1");   // not counted by AllocatePort
            auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
            if (!port) {
                Logger::Log("main", "[IntroSpawner] No UDP ports available for '%s' (%lld, %s)\n",
                    inst->map_name.c_str(), (long long)instance_id, why);
                return false;
            }
            InstanceRegistry::SetPort(instance_id, *port);
            InstanceRegistry::SetAddress(instance_id, "");
            pid_t pid = InstanceSpawner::Spawn(cfg, inst->map_name, inst->game_mode, *port, instance_id);
            if (pid < 0) {
                Logger::Log("main", "[IntroSpawner] Spawn failed for '%s'\n", inst->map_name.c_str());
                return false;
            }
            InstanceRegistry::UpdatePid(instance_id, pid);
            Logger::Log("main", "[IntroSpawner] Spawned '%s' (%s) instance_id=%lld pid=%d port=%d (%s)\n",
                inst->map_name.c_str(), inst->game_mode.c_str(), (long long)instance_id, (int)pid, (int)*port, why);
            return true;
        };
        TcpSession::SetIntroMissionSpawner([&cfg, intro_start_here](const std::string& req_map,
                                                  const std::string& req_mode, IntroHost& host) -> int64_t {
            const std::string map_name  = req_map.empty()  ? cfg.intro_mission_map_name  : req_map;
            const std::string game_mode = req_mode.empty() ? cfg.intro_mission_game_mode : req_mode;
            // Player hosting first - the player's own node gets the chapter
            // even when our server has room (same offer as a one-player match);
            // our server only runs it when the node can't.
            if (host.user_id > 0 && ServiceHooks::Enabled("external_instance")) {
                auto parent = host.parent_instance_id ? InstanceRegistry::GetInstanceById(host.parent_instance_id)
                                                      : std::nullopt;
                ExternalInstances::Owner powner;
                const bool parent_on_node = parent && parent->state != "STOPPED"
                    && ExternalInstances::GetOwner(host.parent_instance_id, powner) && powner.user_id == host.user_id;
                // A node-hosted row holds no port of ours; borrow one when the pool
                // is full, and never the parent's (both maps briefly share the PC).
                auto p = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
                uint16_t port = p ? *p : cfg.udp_port_range.lo;
                if (parent_on_node && port == parent->udp_port)
                    port = (port < cfg.udp_port_range.hi) ? port + 1 : cfg.udp_port_range.lo;
                const int64_t instance_id = InstanceRegistry::InsertStarting(
                    map_name, game_mode, port, 0, /*is_home_map=*/false);
                if (instance_id == 0) return 0;
                // Not on our server (yet): loopback rows hold no pool port, so a
                // local start below can still take any free port, this one too.
                InstanceRegistry::SetAddress(instance_id, "127.0.0.1");
                const nlohmann::json extra = {{"map_name", map_name}, {"game_mode", game_mode},
                    {"udp_port", port}, {"difficulty", 0}, {"players", 1}, {"intro", true}};
                std::string address;
                if (ExternalInstances::Attach(host.user_id, instance_id, host.character_id, extra, &address)) {
                    InstanceRegistry::SetAddress(instance_id, address);
                    ExternalInstances::ArmReadyTimer(instance_id, cfg.external_ready_timeout_s);
                    host.wait_s = cfg.external_ready_timeout_s + 180;
                    Logger::Log("main", "[IntroSpawner] '%s' instance %lld runs externally at %s:%d (user %lld)\n",
                        map_name.c_str(), (long long)instance_id, address.c_str(), (int)port, (long long)host.user_id);
                    return instance_id;
                }
                if (parent_on_node) {
                    // The node is still hosting the chapter being left (one map per
                    // node): close it and offer the next chapter again once the node
                    // is free; the client waits on the loading screen meanwhile.
                    InstanceRegistry::SetAddress(instance_id, parent->ip_address);
                    host.handover = true;
                    const int wait = 30;   // node closes the old map + reconnects its slot
                    host.wait_s = wait + cfg.external_ready_timeout_s + 180;
                    const int64_t parent_id = host.parent_instance_id;
                    const int64_t uid = host.user_id, cid = host.character_id;
                    const int ready_s = cfg.external_ready_timeout_s;
                    Logger::Log("main", "[IntroSpawner] '%s' instance %lld: node of user %lld still hosts %lld -- handover\n",
                        map_name.c_str(), (long long)instance_id, (long long)uid, (long long)parent_id);
                    std::thread([=]() {
                        ExternalInstances::RequestStop(parent_id, "tutorial chapter done");
                        for (int t = 0; t < wait; ++t) {
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                            auto row = InstanceRegistry::GetInstanceById(instance_id);
                            if (!row || row->state != "STARTING") return;   // gone / quit meanwhile
                            std::string addr;
                            if (ExternalInstances::Attach(uid, instance_id, cid, extra, &addr)) {
                                InstanceRegistry::SetAddress(instance_id, addr);
                                ExternalInstances::ArmReadyTimer(instance_id, ready_s);
                                Logger::Log("main", "[IntroSpawner] '%s' instance %lld runs externally at %s:%d (user %lld, after %d s handover)\n",
                                    map_name.c_str(), (long long)instance_id, addr.c_str(), (int)port, (long long)uid, t + 1);
                                return;
                            }
                        }
                        if (!intro_start_here(instance_id, "node did not take the next chapter"))
                            InstanceRegistry::MarkStopped(instance_id);   // -> the player goes home
                    }).detach();
                    return instance_id;
                }
                // Node refused / no node: here, as before (a cap or port shortage -> hub).
                if (!intro_start_here(instance_id, "no player node")) {
                    InstanceRegistry::MarkStopped(instance_id);
                    return 0;
                }
                return instance_id;
            }
            if (!InstanceRegistry::HasFreeMissionSlot()) {
                Logger::Log("main", "[IntroSpawner] instance cap reached - '%s' skipped, agent goes to the hub\n",
                    map_name.c_str());
                return 0;
            }
            auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
            if (!port) {
                Logger::Log("main", "[IntroSpawner] No UDP ports available for '%s'\n", map_name.c_str());
                return 0;
            }
            int64_t instance_id = InstanceRegistry::InsertStarting(
                map_name, game_mode, *port, 0, /*is_home_map=*/false);
            pid_t pid = InstanceSpawner::Spawn(cfg, map_name, game_mode, *port, instance_id);
            if (pid < 0) {
                Logger::Log("main", "[IntroSpawner] Spawn failed for '%s'\n", map_name.c_str());
                InstanceRegistry::MarkStopped(instance_id);
                return 0;
            }
            InstanceRegistry::UpdatePid(instance_id, pid);
            Logger::Log("main", "[IntroSpawner] Spawned '%s' (%s) instance_id=%lld pid=%d port=%d\n",
                map_name.c_str(), game_mode.c_str(), (long long)instance_id, (int)pid, (int)*port);
            return instance_id;
        });
    }

    // Home map spawns on demand when the first player selects a character.
    TcpSession::SetHomeMapSpawner([&cfg]() {
        // Double-check: is there already a non-STOPPED home instance?
        auto existing = InstanceRegistry::GetHomeInstance();
        if (existing) {
            Logger::Log("main", "Home map already exists (instance_id=%lld state=%s) — skipping spawn\n",
                (long long)existing->instance_id, existing->state.c_str());
            return;
        }

        auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
        if (!port) {
            Logger::Log("main", "No UDP ports available for home map spawn\n");
            return;
        }

        int64_t instance_id = InstanceRegistry::InsertStarting(
            cfg.home_map_name, cfg.home_map_game_mode, *port, 0, /*is_home_map=*/true);

        pid_t pid = InstanceSpawner::Spawn(cfg, cfg.home_map_name, cfg.home_map_game_mode, *port, instance_id,
                                           eDifficulty::Default, 0, DifficultyScalar(), cfg.dome_cap);
        if (pid < 0) {
            Logger::Log("main", "Failed to spawn home map instance\n");
            InstanceRegistry::MarkStopped(instance_id);
            return;
        }

        InstanceRegistry::UpdatePid(instance_id, pid);

        Logger::Log("main", "Home map instance spawned on demand: instance_id=%lld pid=%d port=%d\n",
            (long long)instance_id, (int)pid, (int)*port);
    });

    // Hand the io_context to MatchmakingService so it can schedule
    // pop-delay timers. Must happen BEFORE any TCP/chat listener starts
    // (those can trigger AddPlayer which may start a delay timer).
    MatchmakingService::SetIoContext(&io);
    // TeamService schedules its invite-expiry timers on the same io_context.
    TeamService::SetIoContext(&io);

    Logger::Log("main", "%s\n", kGaSourceRevision);
    Logger::Log("main", "Binding TCP listener on port %d\n", (int)cfg.tcp_port);
    // Bind TCP listener (GA client connections)
    TcpListener listener(io, cfg.tcp_port);
    if (!listener.IsListening()) {
        Logger::Log("main", "TCP listener failed; aborting startup\n");
        return 1;
    }

    Logger::Log("main", "Binding chat listener on port %d\n", (int)cfg.chat_port);
    ChatSession::SetAnnouncers(cfg.announcers);
    ChatSession::SetLoginNotice(cfg.login_notice);
    ChatSession::SetNewsFile(cfg.news_file);
    ChatSession::SetDevChatCommands(cfg.dev_controls);
    // Bind chat listener (GA chat connections — separate port)
    ChatListener chat_listener(io, cfg.chat_port);
    if (!chat_listener.IsListening()) {
        Logger::Log("main", "Chat listener failed; aborting startup\n");
        return 1;
    }

    Logger::Log("main", "Binding IPC listener on port %d\n", (int)cfg.ipc_port);
    // Bind IPC server (game instance connections)
    IpcServer ipc_server(io, cfg.ipc_port);
    if (!ipc_server.IsListening()) {
        Logger::Log("main", "IPC listener failed; aborting startup\n");
        return 1;
    }

    // Spectator broadcast overlay HTTP endpoint. Non-fatal if it fails to
    // bind or overlay_http_port is left at its 0 default — the feature is
    // opt-in, unlike the listeners above.
    OverlayHttpServer overlay_http(io, cfg.overlay_bind, cfg.overlay_http_port, cfg.overlay_token);

    Logger::Log("main", "All listeners bound\n");
    IpcServer::SetAdminToken(cfg.admin_token);
    IpcServer::SetAdminActionHandler([&cfg](const std::string& subtype,
                                            const nlohmann::json& payload,
                                            std::string& message) -> bool {
        if (subtype == "stop-instance") {
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            if (instance_id == 0) {
                message = "missing instance_id";
                return false;
            }

            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst) {
                message = "instance not found";
                return false;
            }
            if (inst->state == "STOPPED") {
                message = "instance is already stopped";
                return false;
            }

            Logger::Log("main", "Admin action: stopping instance %lld (pid=%d, map=%s)\n",
                (long long)inst->instance_id, inst->pid, inst->map_name.c_str());
            InstanceSpawner::StopInstanceProcess(*inst, "admin stop-instance");
            InstanceRegistry::MarkStopped(inst->instance_id);
            message = "instance stop requested";
            return true;
        }

        if (subtype == "move-players") {
            int64_t target_instance_id = payload.value("target_instance_id", (int64_t)0);
            int target_task_force = payload.value("target_task_force", 0);
            auto players = payload.value("players", nlohmann::json::array());
            // "from_instance_id" without "players": everyone whose session is in
            // that instance, each keeping the task force of its last MATCH_STATS.
            const int64_t from_instance_id = payload.value("from_instance_id", (int64_t)0);
            // "when_ready": the target may still be starting (see AdminMovePlayerWhenReady).
            const bool when_ready = payload.value("when_ready", false);
            // "title_msg_id" (with when_ready): loading-screen title shown while the target starts.
            const uint32_t title_msg_id = payload.value("title_msg_id", (uint32_t)0);
            if (from_instance_id > 0 && (!players.is_array() || players.empty())) {
                players = nlohmann::json::array();
                TcpSession::ForEachLiveSession([&](const TcpSession::OnlineSnapshot& snap) {
                    if (snap.instance_id != from_instance_id) return;
                    players.push_back({{"session_guid", snap.session_guid},
                        {"source_instance_id", from_instance_id},
                        {"target_task_force", InstanceState::TaskForceOfUser(from_instance_id, snap.user_id)}});
                });
            }
            if (target_instance_id == 0) {
                message = "missing target_instance_id";
                return false;
            }
            if (target_task_force != 1 && target_task_force != 2 && from_instance_id <= 0) {
                message = "target_task_force must be 1 or 2";
                return false;
            }
            if (!players.is_array() || players.empty()) {
                message = "missing players";
                return false;
            }

            int requested = 0;
            int skipped = 0;
            int failed = 0;
            for (const auto& player : players) {
                std::string guid = player.value("session_guid", "");
                if (guid.empty()) {
                    ++skipped;
                    continue;
                }
                int64_t source_instance_id = player.value("source_instance_id", (int64_t)0);
                int source_task_force = player.value("source_task_force", 0);
                int tf = player.value("target_task_force", 0);
                if (tf != 1 && tf != 2) tf = target_task_force;
                if (tf != 1 && tf != 2) tf = 1;
                std::string player_message;
                const bool ok = when_ready
                    ? TcpSession::AdminMovePlayerWhenReady(guid, target_instance_id,
                        tf, source_instance_id, source_task_force, player_message, title_msg_id)
                    : TcpSession::AdminMovePlayerToInstance(guid, target_instance_id,
                        tf, source_instance_id, source_task_force, player_message);
                if (ok) {
                    ++requested;
                } else {
                    ++failed;
                    Logger::Log("main", "Admin move-players failed for %s: %s\n",
                        guid.c_str(), player_message.c_str());
                }
            }

            message = "move requested=" + std::to_string(requested)
                + " failed=" + std::to_string(failed)
                + " skipped=" + std::to_string(skipped);
            return requested > 0 && failed == 0;
        }

        if (subtype == "dev-end-mission") {
            if (!cfg.dev_controls) {
                message = "dev controls are disabled (config dev_controls)";
                return false;
            }
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            std::string result = payload.value("result", std::string("win"));
            if (result != "win" && result != "lose") {
                message = "result must be win|lose";
                return false;
            }
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED") {
                message = "instance not found or stopped";
                return false;
            }
            nlohmann::json msg;
            msg["type"]   = IpcProtocol::MSG_DEV_END_MISSION;
            msg["result"] = result;
            if (!IpcServer::SendToInstance(instance_id, msg.dump())) {
                message = "instance not connected";
                return false;
            }
            Logger::Log("admin", "[admin] dev-end-mission instance=%lld map=%s result=%s\n",
                (long long)instance_id, inst->map_name.c_str(), result.c_str());
            message = "end mission (" + result + ") sent to instance " + std::to_string(instance_id);
            return true;
        }

        if (subtype == "dev-travel") {
            // Dev shortcut: same path as walking into a Map
            // Transition volume (OpenWorldTravel::Request), by username.
            if (!cfg.dev_controls) {
                message = "dev controls are disabled (config dev_controls)";
                return false;
            }
            const std::string username = payload.value("username", std::string());
            const uint32_t map_game_id = payload.value("map_game_id", 0u);
            std::string guid;
            TcpSession::ForEachLiveSession([&](const TcpSession::OnlineSnapshot& s) {
                if (guid.empty() && s.username == username) guid = s.session_guid;
            });
            if (guid.empty() || map_game_id == 0) {
                message = "need a logged-in username and map_game_id";
                return false;
            }
            Logger::Log("admin", "[admin] dev-travel %s -> map_game_id=%u\n",
                username.c_str(), map_game_id);
            OpenWorldTravel::Request(guid, map_game_id);
            message = "travel requested";
            return true;
        }

        if (subtype == "dev-spawn-pvp-bot") {
            // Dev shortcut: same as typing
            // "-spawnpvpbot <class> [friend|enemy]" as `username`.
            if (!cfg.dev_controls) {
                message = "dev controls are disabled (config dev_controls)";
                return false;
            }
            const std::string username = payload.value("username", std::string());
            const std::string side = payload.value("side", std::string("enemy"));
            std::string guid;
            TcpSession::ForEachLiveSession([&](const TcpSession::OnlineSnapshot& s) {
                if (guid.empty() && s.username == username) guid = s.session_guid;
            });
            auto parsed = ChatCommand::TryParseChatCommand(
                "-spawnpvpbot " + payload.value("class", std::string()) + " " + side);
            if (guid.empty() || !parsed.spawn_target) {
                message = "need a logged-in username, class assault|medic|recon|robotics, side friend|enemy";
                return false;
            }
            // Optional "bot_id": dress a different body (A/B against the class bot).
            const int body = payload.value("bot_id", 0);
            if (body > 0) parsed.spawn_target->bot_id = body;
            // Optional "plain": spawn the body undressed (same as -spawnenemy).
            if (payload.value("plain", false)) parsed.spawn_target->pvp_class.clear();
            // Optional "nodev": dress but keep the body's own devices (A/B).
            else if (payload.value("nodev", false)) parsed.spawn_target->pvp_class += "-nodev";
            Logger::Log("admin", "[admin] dev-spawn-pvp-bot %s %s %s body=%d\n", username.c_str(),
                parsed.spawn_target->pvp_class.c_str(), side.c_str(), parsed.spawn_target->bot_id);
            ChatCommand::DispatchSpawnTarget(*parsed.spawn_target, guid);
            message = "pvp bot spawn sent";
            return true;
        }

        if (subtype == "dev-ava-cargo") {  // dev: {instance_id, task_force, bot_carriers, items:[{item_id, deployable_id, count}]}
            if (!cfg.dev_controls) {
                message = "dev controls are disabled (config dev_controls)";
                return false;
            }
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED" || !payload.contains("items") || !payload["items"].is_array()) {
                message = "instance not found/stopped or no items";
                return false;
            }
            nlohmann::json msg;
            msg["type"] = "AVA_CARGO";
            msg["instance_id"] = instance_id;
            msg["sides"] = nlohmann::json::array({{{"task_force", payload.value("task_force", 2)}, {"items", payload["items"]},
                                                 {"bot_carriers", payload.value("bot_carriers", 0)}}});
            const bool ok = IpcServer::SendToInstance(instance_id, msg.dump());
            message = ok ? "ava cargo sent" : "send failed";
            return ok;
        }

        if (subtype == "dev-kill") {
            if (!cfg.dev_controls) {
                message = "dev controls are disabled (config dev_controls)";
                return false;
            }
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED") {
                message = "instance not found or stopped";
                return false;
            }
            nlohmann::json msg;
            msg["type"] = IpcProtocol::MSG_DEV_KILL;
            if (payload.value("target", std::string()) == "bots") {  // kill bots, not players
                msg["target"]  = "bots";
                msg["bot_id"]  = payload.value("bot_id", 0);
                msg["factory"] = payload.value("factory", 0);
            } else if (payload.value("target", std::string()) == "vehicle_hp") {  // AvA vehicle HP / destroy (dev)
                msg["target"]     = "vehicle_hp";
                msg["pct"]        = payload.value("pct", 0);
                msg["task_force"] = payload.value("task_force", 0);
                msg["bot_id"]     = payload.value("bot_id", 0);
            } else if (payload.value("target", std::string()) == "pvpbot_hp") {  // set PvP bot HP (retreat test)
                msg["target"]     = "pvpbot_hp";
                msg["pct"]        = payload.value("pct", 30);
                msg["task_force"] = payload.value("task_force", 0);
            } else if (payload.value("target", std::string()) == "touch") {  // fire a Kismet touch trigger
                msg["target"] = "touch";
                msg["op"]     = payload.value("op", std::string());
                msg["n"]      = payload.value("n", 1);
            }
            if (!IpcServer::SendToInstance(instance_id, msg.dump())) {
                message = "instance not connected";
                return false;
            }
            Logger::Log("admin", "[admin] dev-kill instance=%lld\n", (long long)instance_id);
            message = "kill sent to instance " + std::to_string(instance_id);
            return true;
        }

        if (subtype == "dev-teleport") {
            if (!cfg.dev_controls) {
                message = "dev controls are disabled (config dev_controls)";
                return false;
            }
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED") {
                message = "instance not found or stopped";
                return false;
            }
            nlohmann::json msg;
            msg["type"] = IpcProtocol::MSG_DEV_TELEPORT;
            msg["x"]    = payload.value("x", 0.0);
            msg["y"]    = payload.value("y", 0.0);
            msg["z"]    = payload.value("z", 0.0);
            msg["yaw"]  = payload.value("yaw", 0);
            if (!IpcServer::SendToInstance(instance_id, msg.dump())) {
                message = "instance not connected";
                return false;
            }
            Logger::Log("admin", "[admin] dev-teleport instance=%lld %s\n",
                (long long)instance_id, msg.dump().c_str());
            message = "teleport sent to instance " + std::to_string(instance_id);
            return true;
        }

        if (subtype == "spawn-group" || subtype == "despawn-group" || subtype == "group-status" ||
            subtype == "group-control") {
            // Named bot group at a world position in a running instance (see
            // IpcProtocol SPAWN_GROUP). The instance answers with GROUP_STATUS.
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            const std::string tag = payload.value("tag", std::string());
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED" || tag.empty() || tag.size() > 64) {
                message = "need a running instance_id and a tag (<= 64 chars)";
                return false;
            }
            nlohmann::json msg;
            if (subtype == "spawn-group") {
                if (!payload.contains("bots") || !payload["bots"].is_array() || payload["bots"].empty()) {
                    message = "bots must be a non-empty array of {bot_id, count}";
                    return false;
                }
                msg = payload;
                msg.erase("instance_id");
                msg["type"] = IpcProtocol::MSG_SPAWN_GROUP;
            } else if (subtype == "group-control") {
                // Values are clamped by the instance (WorldGroups::Control).
                msg["type"] = IpcProtocol::MSG_GROUP_CONTROL;
                msg["tag"]  = tag;
                for (const char* k : {"passive", "heal_full", "protect", "native_regen"})
                    if (payload.contains(k) && payload[k].is_boolean()) msg[k] = payload[k];
                for (const char* k : {"regen_frac_s", "home_radius", "settle_s", "max_return_s", "speed_scale",
                                      "regen_cap_frac", "hp_scale", "dmg_scale", "area_radius", "aura_group"})
                    if (payload.contains(k) && payload[k].is_number()) msg[k] = payload[k];
                if (cfg.dev_controls)  // dev/test shortcut only: damage members, credited to a character
                    for (const char* k : {"dev_damage_frac", "dev_credit_char"})
                        if (payload.contains(k) && payload[k].is_number()) msg[k] = payload[k];
            } else {
                msg["type"] = subtype == "despawn-group" ? IpcProtocol::MSG_DESPAWN_GROUP
                                                         : IpcProtocol::MSG_GROUP_STATUS_REQUEST;
                msg["tag"]  = tag;
            }
            if (!IpcServer::SendToInstance(instance_id, msg.dump())) {
                message = "instance not connected";
                return false;
            }
            Logger::Log("admin", "[admin] %s instance=%lld tag=%s\n",
                subtype.c_str(), (long long)instance_id, tag.c_str());
            message = subtype + " sent to instance " + std::to_string(instance_id);
            return true;
        }

        if (subtype == "service-agents") {  // see ServiceAgents.hpp
            nlohmann::json out;
            const bool ok = ServiceAgents::Admin(payload, out, message);
            if (ok) message = out.dump();
            return ok;
        }

        if (subtype == "team-bot") {
            // Task-force bots in a running instance (see IpcProtocol
            // TEAM_BOT). op add {task_force, class, tag, skill?} | remove {tag, slot?, now?}
            // | roster. The instance answers with TEAM_ROSTER (service event
            // "team_roster").
            const int64_t instance_id = payload.value("instance_id", (int64_t)0);
            const std::string op = payload.value("op", std::string());
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED" || (op != "add" && op != "remove" && op != "roster")) {
                message = "need a running instance_id and op add|remove|roster";
                return false;
            }
            nlohmann::json msg;
            msg["type"] = IpcProtocol::MSG_TEAM_BOT;
            msg["op"]   = op;
            for (const char* k : {"class", "tag"})
                if (payload.contains(k) && payload[k].is_string()) msg[k] = payload[k];
            for (const char* k : {"task_force", "slot"})
                if (payload.contains(k) && payload[k].is_number_integer()) msg[k] = payload[k];
            if (payload.contains("now") && payload["now"].is_boolean()) msg["now"] = payload["now"];
            if (payload.contains("skill") && payload["skill"].is_number()) msg["skill"] = payload["skill"];  // 0..1
            if (!IpcServer::SendToInstance(instance_id, msg.dump())) {
                message = "instance not connected";
                return false;
            }
            Logger::Log("admin", "[admin] team-bot %s instance=%lld\n", op.c_str(), (long long)instance_id);
            message = "team-bot " + op + " sent to instance " + std::to_string(instance_id);
            return true;
        }

        if (subtype == "set-world-flag") {
            // {"instance_id": X | "map_prefix": "DomeCity", "flag": "defense_alarm", "value": 0|1}
            // -> SET_WORLD_FLAG to that instance, or to every live instance whose map
            // name starts with map_prefix.
            const std::string flag = payload.value("flag", std::string());
            const std::string prefix = payload.value("map_prefix", std::string());
            const int64_t one = payload.value("instance_id", (int64_t)0);
            if (flag != "defense_alarm" || (one <= 0 && prefix.empty())) {
                message = "need flag=defense_alarm and instance_id or map_prefix";
                return false;
            }
            std::vector<int64_t> ids;
            if (one > 0) ids.push_back(one);
            else if (sqlite3* db = Database::GetConnection()) {
                sqlite3_stmt* s = nullptr;
                if (sqlite3_prepare_v2(db, "SELECT instance_id FROM ga_instances WHERE state != 'STOPPED' "
                        "AND instance_id != 0 AND substr(map_name, 1, length(?1)) = ?1", -1, &s, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(s, 1, prefix.c_str(), -1, SQLITE_TRANSIENT);
                    while (sqlite3_step(s) == SQLITE_ROW) ids.push_back(sqlite3_column_int64(s, 0));
                    sqlite3_finalize(s);
                }
            }
            nlohmann::json msg = {{"type", IpcProtocol::MSG_SET_WORLD_FLAG}, {"flag", flag},
                                  {"value", payload.value("value", 0) ? 1 : 0}};
            int sent = 0;
            for (int64_t id : ids) if (IpcServer::SendToInstance(id, msg.dump())) ++sent;
            Logger::Log("admin", "[admin] set-world-flag %s=%d -> %d/%zu instances\n",
                flag.c_str(), (int)msg["value"], sent, ids.size());
            message = "set-world-flag sent to " + std::to_string(sent) + "/" + std::to_string(ids.size()) + " instances";
            return true;
        }
        if (subtype == "play-fx") {
            // One-shot effect at a world position in a running instance (see
            // IpcProtocol PLAY_FX). Asset paths are checked again in the instance.
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED" || !payload.contains("x") || !payload.contains("y") || !payload.contains("z")) {
                message = "need a running instance_id and x, y, z";
                return false;
            }
            nlohmann::json msg = payload;
            msg.erase("instance_id");
            msg["type"] = IpcProtocol::MSG_PLAY_FX;
            if (!IpcServer::SendToInstance(instance_id, msg.dump())) {
                message = "instance not connected";
                return false;
            }
            Logger::Log("admin", "[admin] play-fx instance=%lld\n", (long long)instance_id);
            message = "play-fx sent to instance " + std::to_string(instance_id);
            return true;
        }
        if (subtype == "place-chest" || subtype == "release-chest") {
            // Borrow / return a placed chest in a running instance (see
            // IpcProtocol PLACE_CHEST / RELEASE_CHEST).
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            const std::string tag = payload.value("tag", std::string());
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            const bool place = subtype == "place-chest";
            if (!inst || inst->state == "STOPPED" || tag.empty()
                || (place && (!payload.contains("x") || !payload.contains("y") || !payload.contains("z")))) {
                message = place ? "need a running instance_id, tag and x, y, z" : "need a running instance_id and tag";
                return false;
            }
            nlohmann::json msg;
            if (place) {
                msg = payload;
                msg.erase("instance_id");
            } else {
                msg["tag"] = tag;
            }
            msg["type"] = place ? IpcProtocol::MSG_PLACE_CHEST : IpcProtocol::MSG_RELEASE_CHEST;
            if (!IpcServer::SendToInstance(instance_id, msg.dump())) {
                message = "instance not connected";
                return false;
            }
            Logger::Log("admin", "[admin] %s instance=%lld tag=%s\n",
                subtype.c_str(), (long long)instance_id, tag.c_str());
            message = subtype + " sent to instance " + std::to_string(instance_id);
            return true;
        }
        if (subtype == "reload-queues") {
            MatchmakingService::ReloadQueues();
            message = "queues reloaded";
            return true;
        }

        if (subtype == "update-queue") {
            uint32_t queue_id = payload.value("queue_id", 0u);
            std::string field = payload.value("field", "");
            if (queue_id == 0) {
                message = "missing queue_id";
                return false;
            }
            if (!payload.contains("value")) {
                message = "missing value";
                return false;
            }
            return UpdateQueueField(queue_id, field, payload["value"], message);
        }

        if (subtype == "update-pool-entry") {
            uint32_t map_pool_id = payload.value("map_pool_id", 0u);
            if (map_pool_id == 0) {
                message = "missing map_pool_id";
                return false;
            }
            return UpdatePoolEntry(map_pool_id,
                payload.value("map_name", std::string()),
                payload.value("game_mode", std::string()),
                payload,
                message);
        }

        if (subtype == "create-instance") {
            return SpawnAdminInstance(cfg,
                payload.value("map_name", std::string()),
                payload.value("game_mode", std::string()),
                payload.value("max_players", 0),
                payload.value("queue_id", 0u),
                message,
                payload.value("group", nlohmann::json::array()),
                payload.value("context", nlohmann::json::object()));
        }

        if (subtype == "notify-players") {
            // {"instance_id": X, "text": "...", "center": bool, "duration": s}
            // -> System chat line to every live session in X; "center" also
            // shows the game's centre-screen alert (needs X to be connected).
            // "all": true -> every live session on the server (instance_id ignored).
            // "center_only_maps": ["Map", ...] -> the centre alert goes only to sessions
            // currently on one of these maps (exact name; no instance yet = chat only).
            // The chat line is unaffected. Absent = centre alert to every session.
            const int64_t instance_id = payload.value("instance_id", (int64_t)0);
            const bool all = payload.value("all", false);
            // optional: one live session only (e.g. a reply to that player's own action)
            const std::string only_guid = payload.value("session_guid", std::string());
            std::string text = payload.value("text", std::string());
            if ((instance_id <= 0 && !all && only_guid.empty()) || text.empty()) {
                message = "missing instance_id (or all or session_guid) or text";
                return false;
            }
            if (text.size() > 200) text = text.substr(0, 200);
            const bool center = payload.value("center", false);
            const double duration = std::max(2.0, std::min(30.0, payload.value("duration", 8.0)));
            const auto map_filter = payload.find("center_only_maps");
            const bool filter_maps = map_filter != payload.end() && map_filter->is_array();
            std::map<int64_t, bool> instance_on_map;
            auto center_allowed = [&](int64_t iid) {
                if (!filter_maps) return true;
                if (iid <= 0) return false;
                auto known = instance_on_map.find(iid);
                if (known != instance_on_map.end()) return known->second;
                bool on_map = false;
                if (auto inst = InstanceRegistry::GetInstanceById(iid))
                    for (const auto& m : *map_filter)
                        if (m.is_string() && m.get<std::string>() == inst->map_name) { on_map = true; break; }
                instance_on_map[iid] = on_map;
                return on_map;
            };
            int chat = 0, alerts = 0;
            std::vector<std::pair<std::string, int64_t>> targets;
            TcpSession::ForEachLiveSession([&](const TcpSession::OnlineSnapshot& snap) {
                if (!only_guid.empty() ? snap.session_guid == only_guid
                                       : (all || snap.instance_id == instance_id))
                    targets.emplace_back(snap.session_guid, snap.instance_id);
            });
            for (const auto& [g, session_instance] : targets) {
                ChatSession::SystemMessageToGuid(g, text);
                ++chat;
                if (!center || !center_allowed(session_instance)) continue;
                nlohmann::json action;
                action["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
                action["session_guid"] = g;
                action["action"]       = "alert_text";
                action["args"] = {{"text", text}, {"priority", 2}, {"type", 1}, {"duration", duration}};
                if (TcpSession::DeliverPlayerAction(g, action)) ++alerts;
            }
            message = "notified chat=" + std::to_string(chat) + " center=" + std::to_string(alerts);
            return true;
        }

        if (subtype == "ban") {
            const std::string target = payload.value("target", std::string());
            const std::string reason = payload.value("reason", std::string());
            if (target != "account" && target != "ip" && target != "both") {
                message = "target must be one of account|ip|both";
                return false;
            }
            if (reason.empty()) {
                message = "reason is required";
                return false;
            }
            int64_t user_id = payload.value("user_id", (int64_t)0);
            const std::string username = payload.value("username", std::string());
            const std::string ip       = payload.value("ip",       std::string());

            if (target == "account" || target == "both") {
                if (user_id == 0 && !username.empty()) {
                    user_id = Database::FindUserIdByUsername(username);
                }
                if (user_id == 0) {
                    message = "account ban needs user_id or a known username";
                    return false;
                }
            }
            if (target == "ip" || target == "both") {
                if (ip.empty()) {
                    message = "ip ban needs ip";
                    return false;
                }
            }

            int kicked = 0;
            if (target == "account" || target == "both") {
                Database::InsertOrReplaceUserBan(user_id, reason);
                Logger::Log("ban", "[ban] admin ban target=account user=%lld reason=\"%s\"\n",
                    (long long)user_id, reason.c_str());
                kicked += TcpSession::KickSessionsForUser(user_id);
            }
            if (target == "ip" || target == "both") {
                Database::InsertOrReplaceIpBan(ip, reason);
                Logger::Log("ban", "[ban] admin ban target=ip ip=%s reason=\"%s\"\n",
                    ip.c_str(), reason.c_str());
                kicked += TcpSession::KickSessionsForIp(ip);
            }

            message = "banned target=" + target + " kicked=" + std::to_string(kicked);
            return true;
        }

        if (subtype == "unban") {
            const std::string target = payload.value("target", std::string());
            if (target != "account" && target != "ip") {
                message = "target must be account or ip";
                return false;
            }
            if (target == "account") {
                int64_t user_id = payload.value("user_id", (int64_t)0);
                if (user_id == 0) {
                    const std::string username = payload.value("username", std::string());
                    if (!username.empty()) user_id = Database::FindUserIdByUsername(username);
                }
                if (user_id == 0) {
                    message = "account unban needs user_id or a known username";
                    return false;
                }
                Database::LiftUserBan(user_id);
                Logger::Log("ban", "[ban] admin unban target=account user=%lld\n", (long long)user_id);
                message = "unbanned account user_id=" + std::to_string(user_id);
                return true;
            }
            const std::string ip = payload.value("ip", std::string());
            if (ip.empty()) {
                message = "ip unban needs ip";
                return false;
            }
            Database::LiftIpBan(ip);
            Logger::Log("ban", "[ban] admin unban target=ip ip=%s\n", ip.c_str());
            message = "unbanned ip=" + ip;
            return true;
        }

        if (subtype == "queue-loot") {
            // {"character_id": C, "loot_table_id": T, "ref": R, "max_required_level": L}:
            // one roll of T for agent C, granted at the agent's next spawn. R = caller's
            // dedupe key. L (optional, 0 = none) = only items needing level <= L.
            const int64_t cid = payload.value("character_id", (int64_t)0);
            const int64_t ref = payload.value("ref", (int64_t)0);
            const int table = payload.value("loot_table_id", 0);
            if (cid <= 0 || ref == 0 || table <= 0) {
                message = "missing character_id, ref or loot_table_id";
                return false;
            }
            message = "queued=" + std::to_string(MissionLoot::QueueForCharacter(cid, ref, table,
                payload.value("max_required_level", 0)));
            return true;
        }

        if (subtype == "grant-currency") {
            // {"character_id": C, "ref": R, "xp": X, "credits": Y, "tokens": Z, "ap": A}:
            // add the amounts to agent C (AP to its account) once per caller key R.
            const int64_t cid = payload.value("character_id", (int64_t)0);
            const int64_t ref = payload.value("ref", (int64_t)0);
            if (cid <= 0 || ref == 0) {
                message = "missing character_id or ref";
                return false;
            }
            const int granted = MissionLoot::GrantCurrency(cid, ref,
                payload.value("xp", (int64_t)0), payload.value("credits", (int64_t)0),
                payload.value("tokens", (int64_t)0), payload.value("ap", (int64_t)0));
            if (granted > 0) {
                // Owner online in a map: push the new wallet to the pawn now.
                int64_t uid = 0;
                sqlite3_stmt* us = nullptr;
                if (sqlite3_prepare_v2(Database::GetConnection(), "SELECT user_id FROM ga_characters WHERE id = ?1",
                        -1, &us, nullptr) == SQLITE_OK) {
                    sqlite3_bind_int64(us, 1, cid);
                    if (sqlite3_step(us) == SQLITE_ROW) uid = sqlite3_column_int64(us, 0);
                    sqlite3_finalize(us);
                }
                std::vector<std::string> guids;
                TcpSession::ForEachLiveSession([&](const TcpSession::OnlineSnapshot& snap) {
                    if (uid > 0 && (int64_t)snap.user_id == uid && snap.instance_id != 0) guids.push_back(snap.session_guid);
                });
                for (const auto& g : guids) {
                    nlohmann::json a;
                    a["type"]         = IpcProtocol::MSG_PLAYER_ACTION;
                    a["session_guid"] = g;
                    a["action"]       = "refresh_wallet";
                    TcpSession::DeliverPlayerAction(g, a);
                }
            }
            message = "granted=" + std::to_string(granted);
            return true;
        }

        if (subtype == "instance-results") {
            // {"action": "list"} or {"action": "commit"|"drop", "instance_id": N}:
            // match results a service deferred at mission end (rating + AvA).
            const std::string action = payload.value("action", "list");
            const int64_t instance_id = payload.value("instance_id", (int64_t)0);
            if (action == "list") { message = NodeClaims::ListResults().dump(); return true; }
            if (action == "commit") return NodeClaims::CommitResults(instance_id, message);
            if (action == "drop") return NodeClaims::DropResults(instance_id, message);
            message = "action must be list, commit or drop";
            return false;
        }

        if (subtype == "instance-snapshot") {
            // {"instance_id": N} -> last reported state (progress + per-player stats) as JSON.
            const auto st = InstanceState::Get(payload.value("instance_id", (int64_t)0));
            message = st.dump();
            return !st.is_null();
        }

        if (subtype == "instance-restore") {
            // {"instance_id": Y, "from_instance_id": X} (last cached state of X) or
            // {"instance_id": Y, "progress": {...}} -> INSTANCE_RESTORE to Y.
            const int64_t instance_id = payload.value("instance_id", (int64_t)0);
            const int64_t from_id = payload.value("from_instance_id", (int64_t)0);
            nlohmann::json progress = payload.value("progress", nlohmann::json());
            if (from_id > 0) {
                const auto st = InstanceState::Get(from_id);
                if (!st.is_null()) progress = st.value("progress", nlohmann::json());
            }
            if (!progress.is_object() || progress.empty()) {
                message = "no state to restore (unknown from_instance_id or empty progress)";
                return false;
            }
            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED") {
                message = "instance not found or stopped";
                return false;
            }
            if (from_id > 0) {
                auto src = InstanceRegistry::GetInstanceById(from_id);
                if (src && src->map_name != inst->map_name) {
                    message = "map differs (" + src->map_name + " -> " + inst->map_name + ")";
                    return false;
                }
            }
            nlohmann::json msg;
            msg["type"]     = IpcProtocol::MSG_INSTANCE_RESTORE;
            msg["progress"] = progress;
            // "when_ready": an instance still starting gets the state on its
            // INSTANCE_READY, ahead of any player registered there.
            if (payload.value("when_ready", false) && inst->state != "READY") {
                InstanceState::Queue(instance_id, msg);
                Logger::Log("admin", "[admin] instance-restore instance=%lld from=%lld queued until READY (state=%s)\n",
                    (long long)instance_id, (long long)from_id, inst->state.c_str());
                message = "state queued for instance " + std::to_string(instance_id);
                return true;
            }
            if (!IpcServer::SendToInstance(instance_id, msg.dump())) {
                message = "instance not connected";
                return false;
            }
            Logger::Log("admin", "[admin] instance-restore instance=%lld from=%lld mode=%s\n",
                (long long)instance_id, (long long)from_id, progress.value("mode", std::string()).c_str());
            message = "state sent to instance " + std::to_string(instance_id);
            return true;
        }

        if (subtype == "external-attach") {
            // {"user_id": N, "instance_id": M[, "character_id": C]} -> ask the external_instance service
            const int64_t user_id = payload.value("user_id", (int64_t)0);
            const int64_t instance_id = payload.value("instance_id", (int64_t)0);
            const bool ok = ExternalInstances::Attach(user_id, instance_id, payload.value("character_id", (int64_t)0));
            message = std::string("external attach ") + (ok ? "accepted" : "refused");
            return ok;
        }

        if (subtype == "pvp-toggle") {
            int64_t user_id = payload.value("user_id", (int64_t)0);
            if (user_id == 0) {
                const std::string username = payload.value("username", std::string());
                if (!username.empty()) user_id = Database::FindUserIdByUsername(username);
            }
            if (user_id == 0) {
                message = "pvp-toggle needs user_id or a known username";
                return false;
            }
            const bool verified = payload.value("verified", false);
            if (!Database::SetUserPvpVerification(user_id, verified)) {
                message = "failed to update verified_for_pvp";
                return false;
            }
            Logger::Log("admin", "[admin] pvp-toggle user_id=%lld verified=%d\n",
                (long long)user_id, verified ? 1 : 0);
            message = std::string("verified_for_pvp set to ") + (verified ? "1" : "0") +
                      " for user_id=" + std::to_string(user_id);
            return true;
        }

        if (subtype == "reset-password") {
            int64_t user_id = payload.value("user_id", (int64_t)0);
            if (user_id == 0) {
                const std::string username = payload.value("username", std::string());
                if (!username.empty()) user_id = Database::FindUserIdByUsername(username);
            }
            if (user_id == 0) {
                message = "reset-password needs user_id or a known username";
                return false;
            }
            if (!Database::ClearUserVerifier(user_id)) {
                message = "failed to clear password verifier";
                return false;
            }
            Logger::Log("admin", "[admin] reset-password user_id=%lld (verifier cleared)\n",
                (long long)user_id);
            message = "password reset for user_id=" + std::to_string(user_id) +
                      " (next login re-registers the password)";
            return true;
        }

        if (subtype == "set-role") {
            // Grants/revokes an entry in ga_user_roles (e.g. role="spectator").
            // No in-game self-service UI — this admin_token-gated action (same
            // local/private-network + token gate as every other admin action)
            // is the only way to grant a role, which is what makes spectator
            // access abuse-resistant: a player can never self-grant it.
            int64_t user_id = payload.value("user_id", (int64_t)0);
            if (user_id == 0) {
                const std::string username = payload.value("username", std::string());
                if (!username.empty()) user_id = Database::FindUserIdByUsername(username);
            }
            if (user_id == 0) {
                message = "set-role needs user_id or a known username";
                return false;
            }
            const std::string role = payload.value("role", std::string());
            if (role.empty()) {
                message = "set-role needs a non-empty role";
                return false;
            }
            const bool granted = payload.value("granted", true);
            if (granted) {
                Database::GrantRole(user_id, role);
            } else {
                Database::RevokeRole(user_id, role);
            }
            Logger::Log("admin", "[admin] set-role user_id=%lld role=%s granted=%d\n",
                (long long)user_id, role.c_str(), granted ? 1 : 0);
            message = "role '" + role + "' " + (granted ? "granted to" : "revoked from") +
                      " user_id=" + std::to_string(user_id);
            return true;
        }

        if (subtype == "set-user-dlc") {
            // Flips an account's installed flag for one ga_dlc pack (string
            // identifier, e.g. "surfside-atoll-pvp-maps"). Used by the dashboard users
            // page and later by the launcher-driven install sync. Takes effect
            // on the player's next GET_TICKET_INFO poll — no relog needed.
            int64_t user_id = payload.value("user_id", (int64_t)0);
            if (user_id == 0) {
                const std::string username = payload.value("username", std::string());
                if (!username.empty()) user_id = Database::FindUserIdByUsername(username);
            }
            if (user_id == 0) {
                message = "set-user-dlc needs user_id or a known username";
                return false;
            }
            const std::string dlc = payload.value("dlc", std::string());
            if (dlc.empty()) {
                message = "set-user-dlc needs a non-empty dlc identifier";
                return false;
            }
            const bool installed = payload.value("installed", false);
            if (!Database::SetUserDlc(user_id, dlc, installed)) {
                message = "unknown dlc identifier '" + dlc + "' or update failed";
                return false;
            }
            Logger::Log("admin", "[admin] set-user-dlc user_id=%lld dlc=%s installed=%d\n",
                (long long)user_id, dlc.c_str(), installed ? 1 : 0);
            message = "dlc '" + dlc + "' " + (installed ? "installed for" : "removed from") +
                      " user_id=" + std::to_string(user_id);
            return true;
        }

        if (subtype == "set-user-note") {
            int64_t user_id = payload.value("user_id", (int64_t)0);
            if (user_id == 0) {
                const std::string username = payload.value("username", std::string());
                if (!username.empty()) user_id = Database::FindUserIdByUsername(username);
            }
            if (user_id == 0) {
                message = "set-user-note needs user_id or a known username";
                return false;
            }
            const std::string note = payload.value("note", std::string());
            if (!Database::SetUserAdminNotes(user_id, note)) {
                message = "failed to update admin_notes";
                return false;
            }
            Logger::Log("admin", "[admin] set-user-note user_id=%lld len=%zu\n",
                (long long)user_id, note.size());
            message = "admin_notes updated for user_id=" + std::to_string(user_id);
            return true;
        }

        if (subtype == "store-ip-check") {
            // Batch upsert of geo/VPN lookup results the dashboard fetched
            // from the external service. The dashboard's DB handle is
            // read-only, so persistence goes through here.
            const auto checks = payload.value("checks", nlohmann::json::array());
            if (!checks.is_array() || checks.empty()) {
                message = "store-ip-check needs a non-empty checks array";
                return false;
            }
            int stored = 0;
            for (const auto& c : checks) {
                if (!c.is_object()) continue;
                const std::string ip = c.value("ip", std::string());
                if (ip.empty()) continue;
                if (Database::UpsertIpCheck(ip,
                        c.value("country_code", std::string()),
                        c.value("country", std::string()),
                        c.value("isp", std::string()),
                        c.value("proxy", false),
                        c.value("hosting", false))) {
                    ++stored;
                }
            }
            Logger::Log("admin", "[admin] store-ip-check stored=%d of %d\n",
                stored, (int)checks.size());
            message = "stored " + std::to_string(stored) + " ip check(s)";
            return stored > 0;
        }

        if (subtype == "list-online") {
            // Snapshot of live TcpSessions for the dashboard "Online now" panel.
            // The admin-action wire format only carries {ok, message}, so we
            // stuff the JSON array into `message` as a string for the
            // dashboard to parse on the Python side.
            nlohmann::json arr = nlohmann::json::array();
            TcpSession::ForEachLiveSession([&arr](const TcpSession::OnlineSnapshot& snap) {
                arr.push_back({
                    {"session_guid", snap.session_guid},
                    {"user_id",      snap.user_id},
                    {"username",     snap.username},
                    {"ip",           snap.ip},
                    {"instance_id",  snap.instance_id},
                    {"login_at",     snap.login_at}
                });
            });
            message = arr.dump();
            return true;
        }

        if (subtype == "mmr-reseed") {
            // Wipes BOTH engines' history and replays every recorded match.
            // Dashboard-only (deliberately no chat command — abuse surface).
            message = MmrService::Reseed();
            return true;
        }

        if (subtype == "set-mmr-engine") {
            const std::string engine = payload.value("engine", "");
            if (!MmrService::SetActiveEngine(engine)) {
                message = "engine must be 'wl' or 'perf'";
                return false;
            }
            message = "active MMR engine set to " + engine;
            return true;
        }

        message = "unknown admin action subtype";
        return false;
    });

    // Market bot round (restock + buy) every tick_seconds; first round 60 s after boot
    auto market_timer = std::make_shared<asio::steady_timer>(io);
    std::function<void(int)> schedule_market;
    schedule_market = [&market_timer, &schedule_market](int secs) {
        market_timer->expires_after(std::chrono::seconds(secs));
        market_timer->async_wait([&schedule_market](std::error_code ec) {
            if (ec) return;
            for (int64_t c : MarketBots::Tick().mailed) TcpSession::NotifyNewMail(c);
            schedule_market(MarketBots::Get().tick_seconds);
        });
    };
    if (MarketBots::Get().enabled) schedule_market(60);

    // AvA season standings refresh + season-end AP payout every 5 min (first 90 s after boot)
    auto season_timer = std::make_shared<asio::steady_timer>(io);
    std::function<void(int)> schedule_season;
    schedule_season = [&season_timer, &schedule_season](int secs) {
        season_timer->expires_after(std::chrono::seconds(secs));
        season_timer->async_wait([&schedule_season](std::error_code ec) {
            if (ec) return;
            TcpSession::AvaSeasonTick((int64_t)std::time(nullptr));
            schedule_season(300);
        });
    };
    schedule_season(90);
    auto ava_region_timer = std::make_shared<asio::steady_timer>(io);
    std::function<void()> schedule_ava_region;
    schedule_ava_region = [&ava_region_timer, &schedule_ava_region]() {
        ava_region_timer->expires_after(std::chrono::seconds(15));
        ava_region_timer->async_wait([&schedule_ava_region](std::error_code ec) {
            if (ec) return;
            TcpSession::AvaRegionTick((int64_t)std::time(nullptr));
            schedule_ava_region();
        });
    };
    schedule_ava_region();

    // Periodic idle instance cleanup (every 60 seconds, kills instances empty for 5+ minutes)
    auto idle_timer = std::make_shared<asio::steady_timer>(io);
    std::function<void()> schedule_idle_check;
    schedule_idle_check = [&io, &idle_timer, &schedule_idle_check]() {
        idle_timer->expires_after(std::chrono::seconds(60));
        idle_timer->async_wait([&io, &idle_timer, &schedule_idle_check](std::error_code ec) {
            if (ec) return;  // timer cancelled (shutdown)

            auto idle = InstanceRegistry::GetIdleInstances(300);  // 5 minutes
            for (const auto& inst : idle) {
                Logger::Log("main", "Idle cleanup: killing instance %lld (pid=%d, map=%s) — empty for 5+ minutes\n",
                    (long long)inst.instance_id, inst.pid, inst.map_name.c_str());
                InstanceSpawner::StopInstanceProcess(inst, "idle cleanup");
                InstanceRegistry::MarkStopped(inst.instance_id);
            }

            // Ghost-player reconciliation: instance-occupancy rows are only
            // cleared by PLAYER_LEFT events, so one lost event (instance
            // hiccup, control-server restart, missed engine reap) leaves a
            // player "in an instance" forever. A row whose session guid has
            // no live control connection is such a ghost: close any leftover
            // engine-side connection (PLAYER_CLOSE → state flip → engine
            // reap) and mark the row left. Idempotent — racing a normal
            // disconnect just repeats no-op cleanup.
            for (const auto& ref : InstanceRegistry::GetAllActiveInstancePlayers()) {
                if (TcpSession::HasLiveSession(ref.session_guid)) continue;
                Logger::Log("main",
                    "Ghost cleanup: guid=%s in instance %lld has no live control session — clearing\n",
                    ref.session_guid.c_str(), (long long)ref.instance_id);
                nlohmann::json close_msg;
                close_msg["type"]           = IpcProtocol::MSG_PLAYER_CLOSE;
                close_msg["session_guid"]   = ref.session_guid;
                close_msg["register_token"] = 0;
                IpcServer::SendToInstance(ref.instance_id, close_msg.dump());
                InstanceRegistry::MarkInstancePlayerLeft(ref.instance_id, ref.session_guid);
            }

            // Same reasoning as the ghost-player reconciliation above, for
            // the spectator overlay's transient state: an instance a
            // spectator visited and then left (with nobody ever polling it
            // again) would otherwise hold those entries until the instance
            // itself stops, which for a still-running match could be a long
            // time. Sweep() prunes stale entries everywhere unconditionally.
            SpectatorOverlayState::Sweep();
            MissionProgressState::Sweep();

            schedule_idle_check();
        });
    };
    schedule_idle_check();

    Logger::Log("main", "Control server ready. TCP port %d, Chat port %d, IPC port %d, DB %s, home_map=%s\n",
        (int)cfg.tcp_port, (int)cfg.chat_port, (int)cfg.ipc_port, cfg.db_path.c_str(), cfg.home_map_name.c_str());

    // Run event loop -- blocks until io.stop() or all work complete.
    // INSTANCE_READY from the game DLL will call InstanceRegistry::MarkReady() asynchronously.
    io.run();

    Logger::Log("main", "Control server shutting down — terminating spawned instances.\n");

    // Stop every spawned game-instance before exit. InstanceSpawner owns
    // platform-specific process teardown: Linux uses process groups; Windows
    // terminates the tracked child process handle by PID.
    {
        auto running = InstanceRegistry::GetAllRunningInstances();
        Logger::Log("main", "  %zu running instance(s) to terminate\n", running.size());
        for (const auto& inst : running) {
            InstanceSpawner::StopInstanceProcess(inst, "control-server shutdown", 2);
            InstanceRegistry::MarkStopped(inst.instance_id);
        }
    }

    Logger::Log("main", "Control server shutdown complete.\n");
    g_io = nullptr;

    return 0;
}
