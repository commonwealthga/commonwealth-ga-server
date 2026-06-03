#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/MatchmakingService/RuleFactory.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <limits>
#include <random>

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------

std::unordered_map<uint32_t, MatchmakingService::Queue> MatchmakingService::queues_;
MatchmakingService::MatchPopCallback MatchmakingService::on_match_pop_;
MatchmakingService::InstanceProvider MatchmakingService::instance_provider_;
std::unordered_map<int64_t, PendingMatch> MatchmakingService::pending_matches_;
std::unordered_map<int64_t, std::unordered_map<std::string, int>>
    MatchmakingService::pre_assigned_teams_;
asio::io_context* MatchmakingService::io_ctx_ = nullptr;

// ---------------------------------------------------------------------------
// Init / reload
// ---------------------------------------------------------------------------

// Shared RNG for PickRandomMapPoolEntryForCount and BalancedPvp shuffle.
// Control server runs single-threaded asio so no mutex needed.
static std::mt19937& MatchRng() {
    static std::mt19937 eng{std::random_device{}()};
    return eng;
}

static TaskforcePolicy ParseTaskforcePolicy(const std::string& s) {
    if (s == "pinned_1")     return TaskforcePolicy::Pinned1;
    if (s == "pinned_2")     return TaskforcePolicy::Pinned2;
    if (s == "balanced")     return TaskforcePolicy::Balanced;
    if (s == "balanced_pvp") return TaskforcePolicy::BalancedPvp;
    Logger::Log("matchmaking",
        "[Matchmaking] Unknown taskforce_policy '%s' — defaulting to pinned_1\n", s.c_str());
    return TaskforcePolicy::Pinned1;
}

// Pull every row in ga_queues + ga_map_pool_entries into in-memory QueueConfigs.
// Disabled queues are still loaded so GetQueueConfig works; only
// GetEnabledQueueConfigs filters them out for the ticket-info wire.
static std::vector<QueueConfig> LoadAllQueueConfigsFromDb() {
    std::vector<QueueConfig> out;
    sqlite3* db = Database::GetConnection();
    if (!db) return out;

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT queue_id, map_pool_id, name, rule_class, taskforce_policy, continue_in_queue, enabled,"
        "       queue_type_value_id, status_msg_id, name_msg_id, desc_msg_id, icon_id,"
        "       max_players_per_side, min_players_per_team, max_players_per_team,"
        "       level_min, level_max, tab, map_x, map_y, map_active_flag,"
        "       map_icon_texture_res_id, video_res_id, location_value_id, double_agent_flag,"
        "       sys_site_id, sort_order, bonus_queue_flag, difficulty_value_id,"
        "       access_flags, active_flag, locked_flag, remaining_seconds,"
        "       min_players_to_pop, max_players_per_instance, pop_delay_seconds "
        "FROM ga_queues ORDER BY sort_order, queue_id";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("matchmaking", "[Matchmaking] LoadQueueConfigs prepare failed: %s\n",
            sqlite3_errmsg(db));
        return out;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QueueConfig c;
        int col = 0;
        c.queue_id = (uint32_t)sqlite3_column_int(stmt, col++);
        c.map_pool_id = (sqlite3_column_type(stmt, col) == SQLITE_NULL)
                        ? 0u
                        : (uint32_t)sqlite3_column_int(stmt, col);
        col++;
        if (auto* p = sqlite3_column_text(stmt, col++)) c.name = (const char*)p;
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) {
            c.rule_class = (const char*)sqlite3_column_text(stmt, col);
        }
        col++;
        c.taskforce_policy = ParseTaskforcePolicy(
            sqlite3_column_text(stmt, col) ? (const char*)sqlite3_column_text(stmt, col) : "");
        col++;
        c.continue_in_queue = sqlite3_column_int(stmt, col++) != 0;
        c.enabled           = sqlite3_column_int(stmt, col++) != 0;
        c.queue_type_value_id     = (uint32_t)sqlite3_column_int(stmt, col++);
        c.status_msg_id           = (uint32_t)sqlite3_column_int(stmt, col++);
        c.name_msg_id             = (uint32_t)sqlite3_column_int(stmt, col++);
        c.desc_msg_id             = (uint32_t)sqlite3_column_int(stmt, col++);
        c.icon_id                 = (uint32_t)sqlite3_column_int(stmt, col++);
        c.max_players_per_side    = (uint32_t)sqlite3_column_int(stmt, col++);
        c.min_players_per_team    = (uint32_t)sqlite3_column_int(stmt, col++);
        c.max_players_per_team    = (uint32_t)sqlite3_column_int(stmt, col++);
        c.level_min               = (uint32_t)sqlite3_column_int(stmt, col++);
        c.level_max               = (uint32_t)sqlite3_column_int(stmt, col++);
        c.tab                     = (uint32_t)sqlite3_column_int(stmt, col++);
        c.map_x                   = (float)sqlite3_column_double(stmt, col++);
        c.map_y                   = (float)sqlite3_column_double(stmt, col++);
        c.map_active_flag         = sqlite3_column_int(stmt, col++) != 0;
        c.map_icon_texture_res_id = (uint32_t)sqlite3_column_int(stmt, col++);
        c.video_res_id            = (uint32_t)sqlite3_column_int(stmt, col++);
        c.location_value_id       = (uint32_t)sqlite3_column_int(stmt, col++);
        c.double_agent_flag       = sqlite3_column_int(stmt, col++) != 0;
        c.sys_site_id             = (uint32_t)sqlite3_column_int(stmt, col++);
        c.sort_order              = (uint32_t)sqlite3_column_int(stmt, col++);
        c.bonus_queue_flag        = sqlite3_column_int(stmt, col++) != 0;
        c.difficulty_value_id     = (uint32_t)sqlite3_column_int(stmt, col++);
        c.access_flags            = (uint64_t)sqlite3_column_int64(stmt, col++);
        c.active_flag             = sqlite3_column_int(stmt, col++) != 0;
        c.locked_flag             = sqlite3_column_int(stmt, col++) != 0;
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL) {
            c.remaining_seconds = (uint32_t)sqlite3_column_int(stmt, col);
        }
        col++;
        c.min_players_to_pop = (uint32_t)sqlite3_column_int(stmt, col++);
        if (c.min_players_to_pop == 0) {
            Logger::Log("matchmaking",
                "[Matchmaking] Queue %u min_players_to_pop=0 invalid — clamping to 1\n",
                c.queue_id);
            c.min_players_to_pop = 1;
        }
        c.max_players_per_instance = (uint32_t)sqlite3_column_int(stmt, col++);
        c.pop_delay_seconds        = (float)sqlite3_column_double(stmt, col++);
        out.push_back(std::move(c));
    }
    sqlite3_finalize(stmt);

    // Pool join. One pass per queue keeps the SQL trivial; pool sizes are
    // small (typically <40 entries). Queues with map_pool_id=0 (unassigned)
    // load with an empty map_pool and matchmaker skips them on spawn.
    for (auto& c : out) {
        if (c.map_pool_id == 0) continue;
        sqlite3_stmt* ps = nullptr;
        const char* psql =
            "SELECT map_name, game_mode, weight, min_players, max_players "
            "FROM ga_map_pool_entries "
            "WHERE map_pool_id = ? AND enabled = 1";
        if (sqlite3_prepare_v2(db, psql, -1, &ps, nullptr) != SQLITE_OK || !ps) continue;
        sqlite3_bind_int(ps, 1, (int)c.map_pool_id);
        while (sqlite3_step(ps) == SQLITE_ROW) {
            MapModeEntry m;
            if (auto* p = sqlite3_column_text(ps, 0)) m.map_name  = (const char*)p;
            if (auto* p = sqlite3_column_text(ps, 1)) m.game_mode = (const char*)p;
            m.weight = std::max(1, sqlite3_column_int(ps, 2));
            if (sqlite3_column_type(ps, 3) != SQLITE_NULL) {
                m.min_players = sqlite3_column_int(ps, 3);
            }
            if (sqlite3_column_type(ps, 4) != SQLITE_NULL) {
                m.max_players = sqlite3_column_int(ps, 4);
            }
            c.map_pool.push_back(std::move(m));
        }
        sqlite3_finalize(ps);
    }

    return out;
}

void MatchmakingService::Init() {
    queues_.clear();
    pending_matches_.clear();
    pre_assigned_teams_.clear();
    on_match_pop_ = nullptr;
    instance_provider_ = nullptr;
    io_ctx_ = nullptr;
    Logger::Log("matchmaking", "[Matchmaking] Initialized — loading queue configs from DB\n");

    for (auto& cfg : LoadAllQueueConfigsFromDb()) {
        const uint32_t qid = cfg.queue_id;
        queues_[qid] = BuildQueue(std::move(cfg));
        Logger::Log("matchmaking",
            "[Matchmaking] Loaded queue %u '%s' (rule=%s pool_id=%u pool_size=%zu)\n",
            qid, queues_[qid].config.name.c_str(),
            queues_[qid].config.rule_class.empty() ? "DataDriven" : queues_[qid].config.rule_class.c_str(),
            queues_[qid].config.map_pool_id,
            queues_[qid].config.map_pool.size());
    }
}

void MatchmakingService::ReloadQueues() {
    auto fresh = LoadAllQueueConfigsFromDb();

    // Move out the existing queues' player lists AND delayed_pop state
    // keyed by queue_id so we can re-attach them to the rebuilt Queue.
    // Players in queues that disappeared from the DB are dropped with
    // a log line — intentional: a removed queue can no longer route them
    // anywhere. Surviving delayed_pop timers carry their current
    // next_duration / fires_at unchanged across the reload.
    std::unordered_map<uint32_t, std::vector<QueuedPlayer>> kept_players;
    std::unordered_map<uint32_t, std::optional<DelayedPop>> kept_delays;
    for (auto& [qid, q] : queues_) {
        kept_players[qid] = std::move(q.players);
        kept_delays[qid]  = std::move(q.delayed_pop);
    }

    std::unordered_map<uint32_t, Queue> rebuilt;
    rebuilt.reserve(fresh.size());
    for (auto& cfg : fresh) {
        const uint32_t qid = cfg.queue_id;
        Queue q = BuildQueue(std::move(cfg));
        auto pit = kept_players.find(qid);
        if (pit != kept_players.end()) {
            q.players = std::move(pit->second);
            kept_players.erase(pit);
        }
        auto dit = kept_delays.find(qid);
        if (dit != kept_delays.end()) {
            q.delayed_pop = std::move(dit->second);
            kept_delays.erase(dit);
        }
        rebuilt[qid] = std::move(q);
    }

    for (const auto& [qid, players] : kept_players) {
        if (players.empty()) continue;
        Logger::Log("matchmaking",
            "[Matchmaking] ReloadQueues: queue %u removed — dropped %zu queued player(s)\n",
            qid, players.size());
    }

    // Cancel any timers belonging to queues that no longer exist so their
    // completion handlers don't fire against a missing Queue.
    for (auto& [qid, dp] : kept_delays) {
        if (dp && dp->timer) {
            dp->timer->cancel();
            Logger::Log("queue-pop",
                "[Matchmaking] DelayedPop cancelled queue=%u reason=queue_removed (ReloadQueues)\n",
                qid);
        }
    }

    queues_ = std::move(rebuilt);
    Logger::Log("matchmaking", "[Matchmaking] ReloadQueues: %zu queue(s) now active\n", queues_.size());

    // Pending matches retain their pre-reload task_force_assignments and
    // session_guids — they continue routing on INSTANCE_READY untouched.
}

// ---------------------------------------------------------------------------
// Queue lifecycle helpers
// ---------------------------------------------------------------------------

MatchmakingService::Queue MatchmakingService::BuildQueue(QueueConfig cfg) {
    Queue q;
    q.config = std::move(cfg);
    q.rule = RuleFactory::Create(&q.config);
    return q;
}

bool MatchmakingService::GetContinueInQueue(uint32_t queue_id) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) return false;
    return it->second.config.continue_in_queue;
}

std::vector<QueueConfig> MatchmakingService::GetEnabledQueueConfigs() {
    std::vector<QueueConfig> out;
    out.reserve(queues_.size());
    for (const auto& [qid, q] : queues_) {
        if (q.config.enabled) out.push_back(q.config);
    }
    std::sort(out.begin(), out.end(),
        [](const QueueConfig& a, const QueueConfig& b) {
            if (a.sort_order != b.sort_order) return a.sort_order < b.sort_order;
            return a.queue_id < b.queue_id;
        });
    return out;
}

std::optional<QueueConfig> MatchmakingService::GetQueueConfig(uint32_t queue_id) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) return std::nullopt;
    return it->second.config;
}

MatchmakingService::QueuedProfileCounts
MatchmakingService::GetQueuedProfileCounts(uint32_t queue_id) {
    QueuedProfileCounts counts{0, 0, 0, 0};

    auto tally = [&](uint32_t profile_id) {
        switch (profile_id) {
            case 680: counts.assault++;  break;  // PROFILE_ASSAULT
            case 567: counts.medic++;    break;  // PROFILE_MEDIC
            case 681: counts.recon++;    break;  // PROFILE_RECON
            case 679: counts.robotics++; break;  // PROFILE_ROBOTICS
            default: break;
        }
    };

    // Players still sitting in queue.players. In practice this is almost
    // always empty by the time GET_TICKET_INFO runs — AddPlayer immediately
    // calls TryPop which pops them into a pending match (or instantly into
    // a READY existing instance). Kept for completeness in case future rules
    // gain min-player thresholds that hold players in the queue.
    auto it = queues_.find(queue_id);
    if (it != queues_.end()) {
        for (const auto& p : it->second.players) tally(p.profile_id);
    }

    // Players who've been popped into a pending match but haven't yet
    // registered into the running instance (INSTANCE_READY hasn't fired
    // yet, or it has but PLAYER_REGISTER_ACK hasn't completed). These are
    // the "waiting for the match to start" players the queue card should
    // show — without this branch GetQueuedProfileCounts was effectively
    // always zero. ga_instance_players covers the post-register phase via
    // InstanceRegistry::GetActiveProfileCountsForQueue.
    for (const auto& [iid, pm] : pending_matches_) {
        if (pm.queue_id != queue_id) continue;
        for (const auto& [guid, pid] : pm.profile_ids) tally(pid);
    }

    return counts;
}

std::optional<uint32_t>
MatchmakingService::GetDelayedPopRemainingSeconds(uint32_t queue_id) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) return std::nullopt;
    if (!it->second.delayed_pop) return std::nullopt;
    auto now = std::chrono::steady_clock::now();
    auto delta = it->second.delayed_pop->fires_at - now;
    if (delta <= std::chrono::milliseconds(0)) return std::nullopt;
    // Round up to whole seconds so a 0.001s remainder shows as "1s" rather
    // than vanishing — the wire field can't carry sub-second precision.
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();
    return (uint32_t)((ms + 999) / 1000);
}

std::optional<MapModeSpec>
MatchmakingService::PickRandomMapPoolEntryForCount(uint32_t queue_id, int count) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) {
        Logger::Log("matchmaking",
            "[Matchmaking] PickRandomMapPoolEntryForCount: queue %u not registered\n", queue_id);
        return std::nullopt;
    }
    const auto& pool = it->second.config.map_pool;
    if (pool.empty()) {
        Logger::Log("matchmaking",
            "[Matchmaking] PickRandomMapPoolEntryForCount: queue %u has empty map_pool\n", queue_id);
        return std::nullopt;
    }

    auto matches = [count](const MapModeEntry& e) {
        if (e.min_players && count < *e.min_players) return false;
        if (e.max_players && count > *e.max_players) return false;
        return true;
    };

    // First pass: strict window match.
    std::vector<const MapModeEntry*> candidates;
    for (const auto& e : pool) {
        if (matches(e)) candidates.push_back(&e);
    }

    bool nearest_fit_used = false;
    int  best_distance    = 0;

    if (candidates.empty()) {
        // Nearest-fit fallback. window_distance = boundary→count delta;
        // 0 means in-window (would have matched strictly).
        nearest_fit_used = true;
        auto window_distance = [count](const MapModeEntry& e) -> int {
            if (e.min_players && count < *e.min_players) return *e.min_players - count;
            if (e.max_players && count > *e.max_players) return count - *e.max_players;
            return 0;
        };
        best_distance = std::numeric_limits<int>::max();
        for (const auto& e : pool) {
            const int d = window_distance(e);
            if (d < best_distance) {
                best_distance = d;
                candidates.clear();
                candidates.push_back(&e);
            } else if (d == best_distance) {
                candidates.push_back(&e);
            }
        }
    }

    if (candidates.empty()) return std::nullopt;  // unreachable given non-empty pool

    int total = 0;
    for (const auto* e : candidates) total += e->weight;
    std::uniform_int_distribution<int> dist(0, total - 1);
    int roll = dist(MatchRng());
    const MapModeEntry* picked = candidates.back();
    for (const auto* e : candidates) {
        roll -= e->weight;
        if (roll < 0) { picked = e; break; }
    }

    if (nearest_fit_used) {
        Logger::Log("queue-pop",
            "[Matchmaking] map_pool nearest queue=%u count=%d candidates=%zu/%zu picked=%s distance=%d\n",
            queue_id, count, candidates.size(), pool.size(),
            picked->map_name.c_str(), best_distance);
    } else {
        Logger::Log("queue-pop",
            "[Matchmaking] map_pool filter queue=%u count=%d candidates=%zu/%zu picked=%s in_window\n",
            queue_id, count, candidates.size(), pool.size(),
            picked->map_name.c_str());
    }

    return MapModeSpec{ picked->map_name, picked->game_mode };
}

void MatchmakingService::SetMatchPopCallback(MatchPopCallback cb) {
    on_match_pop_ = std::move(cb);
}

void MatchmakingService::SetInstanceProvider(InstanceProvider provider) {
    instance_provider_ = std::move(provider);
}

void MatchmakingService::SetIoContext(asio::io_context* io) {
    io_ctx_ = io;
}

// ---------------------------------------------------------------------------
// Player management
// ---------------------------------------------------------------------------

void MatchmakingService::AddPlayer(uint32_t queue_id, const QueuedPlayer& player) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) {
        Logger::Log("matchmaking", "[Matchmaking] AddPlayer: unknown queue %u\n", queue_id);
        return;
    }

    it->second.players.push_back(player);
    Logger::Log("matchmaking", "[Matchmaking] Player %s joined queue %u (%zu players)\n",
        player.session_guid.c_str(), queue_id, it->second.players.size());

    if (it->second.delayed_pop) {
        // Active wait: halve+reset directly. TryPop will re-enter via the
        // timer's completion handler with delay_elapsed=true.
        MaybeResetDelayedPop(it->second, "join", player.session_guid);
    } else {
        TryPop(queue_id);
    }
}

void MatchmakingService::RemovePlayer(const std::string& session_guid) {
    // 1. queue.players — almost always empty by the time MATCH_LEAVE arrives
    // because TryPop pops the player into a pending match on join. Kept for
    // future rules with min-player thresholds that hold players in the queue.
    for (auto& [queue_id, queue] : queues_) {
        auto& players = queue.players;
        auto it = std::find_if(players.begin(), players.end(),
            [&](const QueuedPlayer& p) { return p.session_guid == session_guid; });
        if (it != players.end()) {
            Logger::Log("matchmaking", "[Matchmaking] Player %s removed from queue %u\n",
                session_guid.c_str(), queue_id);
            players.erase(it);
            // If a pop-delay wait is in flight for this queue, halve+reset
            // (or cancel if we dropped below min_players_to_pop).
            if (queue.delayed_pop) {
                MaybeResetDelayedPop(queue, "leave", session_guid);
            }
            // Don't return — the player might also be in a pending match if a
            // race window allowed both (defensive sweep below).
            break;
        }
    }

    // 2. pending_matches_ — the common case. Scrub the guid from session_guids
    // plus the task_force_assignments/profile_ids maps so the queue card no
    // longer shows the player as queued. Pending matches that go empty are
    // left in place: they're a valid landing slot for future coalesce, and
    // ConsumePendingMatch at INSTANCE_READY tolerates an empty session list
    // (no invitations sent → instance idle-dies via the normal cleanup path).
    for (auto& [iid, pm] : pending_matches_) {
        auto sit = std::find(pm.session_guids.begin(), pm.session_guids.end(), session_guid);
        if (sit == pm.session_guids.end()) continue;
        pm.session_guids.erase(sit);
        pm.task_force_assignments.erase(session_guid);
        pm.profile_ids.erase(session_guid);
        Logger::Log("matchmaking",
            "[Matchmaking] Player %s removed from pending match for instance %lld (%zu player(s) left)\n",
            session_guid.c_str(), (long long)iid, pm.session_guids.size());
        break;  // a guid only ever exists in one pending entry
    }
}

// ---------------------------------------------------------------------------
// Pending match management
// ---------------------------------------------------------------------------

void MatchmakingService::AddPendingMatch(int64_t instance_id, PendingMatch match) {
    Logger::Log("matchmaking", "[Matchmaking] Pending match added for instance %lld (%zu players)\n",
        (long long)instance_id, match.session_guids.size());
    pending_matches_[instance_id] = std::move(match);
}

std::optional<PendingMatch> MatchmakingService::ConsumePendingMatch(int64_t instance_id) {
    auto it = pending_matches_.find(instance_id);
    if (it == pending_matches_.end()) return std::nullopt;
    PendingMatch match = std::move(it->second);
    pending_matches_.erase(it);
    return match;
}

void MatchmakingService::SetPreAssignedTeam(int64_t instance_id,
                                            const std::string& guid, int tf) {
    pre_assigned_teams_[instance_id][guid] = tf;
    Logger::Log("matchmaking",
        "[Matchmaking] PreAssignedTeam set: instance=%lld guid=%s tf=%d\n",
        (long long)instance_id, guid.c_str(), tf);
}

std::optional<int>
MatchmakingService::ConsumePreAssignedTeam(int64_t instance_id,
                                           const std::string& guid) {
    auto it_inst = pre_assigned_teams_.find(instance_id);
    if (it_inst == pre_assigned_teams_.end()) return std::nullopt;
    auto it_guid = it_inst->second.find(guid);
    if (it_guid == it_inst->second.end()) return std::nullopt;
    int tf = it_guid->second;
    it_inst->second.erase(it_guid);
    if (it_inst->second.empty()) pre_assigned_teams_.erase(it_inst);
    Logger::Log("matchmaking",
        "[Matchmaking] PreAssignedTeam consumed: instance=%lld guid=%s tf=%d\n",
        (long long)instance_id, guid.c_str(), tf);
    return tf;
}

void MatchmakingService::DropPreAssignedTeams(int64_t instance_id) {
    auto it = pre_assigned_teams_.find(instance_id);
    if (it == pre_assigned_teams_.end()) return;
    size_t n = it->second.size();
    pre_assigned_teams_.erase(it);
    if (n > 0) {
        Logger::Log("matchmaking",
            "[Matchmaking] PreAssignedTeams dropped: instance=%lld n=%zu\n",
            (long long)instance_id, n);
    }
}

void MatchmakingService::DiscardPendingMatchForDeadInstance(int64_t instance_id, const char* reason) {
    auto it = pending_matches_.find(instance_id);
    if (it == pending_matches_.end()) return;  // no-op — instance died after READY

    Logger::Log("matchmaking",
        "[Matchmaking] Discarding pending match for dead instance %lld (%zu player(s), reason: %s)\n",
        (long long)instance_id, it->second.session_guids.size(), reason ? reason : "unspecified");

    // Unstick each matched player BEFORE erasing — DeliverMatchCancelled
    // is silent if the session has gone away, so order doesn't matter for
    // correctness; doing it first makes the log lines easier to read.
    for (const auto& guid : it->second.session_guids) {
        TcpSession::DeliverMatchCancelled(guid, reason);
    }
    pending_matches_.erase(it);
}

// Helper: is this instance still in an active (non-STOPPED) state? Used by
// TryPop's coalesce loop as belt-and-suspenders against the pending-match
// outliving a crashed instance. Cheap DB query — pending_matches_ usually
// has 0-2 entries per queue.
static bool IsInstanceActive(int64_t instance_id) {
    auto info = InstanceRegistry::GetInstanceById(instance_id);
    return info && info->state != "STOPPED";
}

// ---------------------------------------------------------------------------
// Core logic
// ---------------------------------------------------------------------------

void MatchmakingService::MaybeResetDelayedPop(Queue& q,
                                              const char* trigger,
                                              const std::string& guid) {
    if (!q.delayed_pop) return;

    // Drop below min_players → cancel the wait entirely.
    if (q.players.size() < q.config.min_players_to_pop) {
        if (q.delayed_pop->timer) q.delayed_pop->timer->cancel();
        Logger::Log("queue-pop",
            "[Matchmaking] DelayedPop cancelled queue=%u reason=below_min (players=%zu < min=%u)\n",
            q.config.queue_id, q.players.size(), q.config.min_players_to_pop);
        q.delayed_pop.reset();
        return;
    }

    const float prev = q.delayed_pop->next_duration;
    const float next = std::max(0.5f, prev * 0.5f);
    q.delayed_pop->next_duration = next;

    const auto next_ms = std::chrono::milliseconds((int64_t)(next * 1000.0f));
    q.delayed_pop->timer->expires_after(next_ms);
    q.delayed_pop->fires_at = std::chrono::steady_clock::now() + next_ms;

    const uint32_t qid = q.config.queue_id;
    auto timer = q.delayed_pop->timer;
    q.delayed_pop->timer->async_wait([qid, timer](const asio::error_code& ec) {
        if (ec) return;
        auto it = queues_.find(qid);
        if (it == queues_.end()) return;
        if (!it->second.delayed_pop
                || it->second.delayed_pop->timer != timer) return;
        it->second.delayed_pop.reset();
        Logger::Log("queue-pop",
            "[Matchmaking] DelayedPop fired queue=%u — proceeding to spawn\n", qid);
        TryPop(qid, /*delay_elapsed=*/true);
    });

    Logger::Log("queue-pop",
        "[Matchmaking] DelayedPop reset queue=%u prev=%.2fs new=%.2fs players=%zu trigger=%s guid=%s\n",
        q.config.queue_id, prev, next, q.players.size(), trigger, guid.c_str());
}

void MatchmakingService::TryPop(uint32_t queue_id, bool delay_elapsed) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) return;

    auto& queue = it->second;
    if (!queue.rule || queue.players.empty()) return;

    // Queue-scoped instance list — the provider filters by queue_id so rules
    // don't see (or accidentally route into) instances from other queues.
    std::vector<RunningInstance> instances;
    if (instance_provider_) {
        instances = instance_provider_(queue_id);
    }

    auto result = queue.rule->Evaluate(queue.players, instances);
    if (!result) return;

    // Gate spawning new instances on min_players_to_pop. Does NOT apply
    // when the rule chose to join an existing READY instance — that
    // instance already met its threshold once.
    if (!result->existing_instance_id
            && queue.players.size() < queue.config.min_players_to_pop) {
        Logger::Log("queue-pop",
            "[Matchmaking] TryPop queue=%u below min_players_to_pop "
            "(have=%zu need=%u) — waiting for more joins\n",
            queue_id, queue.players.size(), queue.config.min_players_to_pop);
        return;
    }

    // Pop-delay gate. Applies to spawn-new only (existing-instance joins
    // skip this whole block because the rule returned an existing_instance_id
    // and the coalesce/spawn section below short-circuits anyway). Invariant:
    // queue.delayed_pop is inactive on entry — AddPlayer/RemovePlayer call
    // MaybeResetDelayedPop directly when active, never re-enter TryPop.
    if (!delay_elapsed
            && !result->existing_instance_id
            && queue.config.pop_delay_seconds > 0.0f
            && io_ctx_) {
        const auto now = std::chrono::steady_clock::now();
        const auto dur_ms = std::chrono::milliseconds(
            (int64_t)(queue.config.pop_delay_seconds * 1000.0f));

        DelayedPop dp;
        dp.next_duration = queue.config.pop_delay_seconds;
        dp.timer = std::make_shared<asio::steady_timer>(*io_ctx_);
        dp.timer->expires_after(dur_ms);
        dp.fires_at = now + dur_ms;

        const uint32_t qid = queue_id;
        auto timer = dp.timer;
        dp.timer->async_wait([qid, timer](const asio::error_code& ec) {
            if (ec) return;  // cancelled
            auto it = queues_.find(qid);
            if (it == queues_.end()) return;
            if (!it->second.delayed_pop
                    || it->second.delayed_pop->timer != timer) return;
            it->second.delayed_pop.reset();
            Logger::Log("queue-pop",
                "[Matchmaking] DelayedPop fired queue=%u — proceeding to spawn\n", qid);
            TryPop(qid, /*delay_elapsed=*/true);
        });

        Logger::Log("queue-pop",
            "[Matchmaking] DelayedPop started queue=%u duration=%.2fs players=%zu\n",
            queue_id, queue.config.pop_delay_seconds, queue.players.size());

        queue.delayed_pop = std::move(dp);
        return;
    }

    // Rule signaled "spawn new" without committing to a map — fill from pool.
    // (Rules can still hardcode a map by setting result.map_name themselves;
    // we only overwrite when both map_name AND existing_instance_id are unset.)
    if (!result->existing_instance_id && result->map_name.empty()) {
        auto picked = PickRandomMapPoolEntryForCount(
            queue_id, (int)result->session_guids.size());
        if (!picked) {
            Logger::Log("matchmaking",
                "[Matchmaking] Queue %u rule wants new spawn but map_pool is empty/unconfigured — skipping pop\n",
                queue_id);
            return;
        }
        result->map_name  = picked->map_name;
        result->game_mode = picked->game_mode;
    }

    // Coalesce-into-pending check. If the rule wants a fresh spawn but we
    // already have a pending match for this queue (an instance that was
    // spawned by a recent pop and hasn't reported INSTANCE_READY yet), append
    // the new players to that pending match instead of spawning a duplicate.
    //
    // This prevents the classic race: N players hit a cold queue
    // simultaneously, instance_provider_ only returns READY instances, so
    // every TryPop after the first sees an empty instance list and the rule
    // keeps asking for a fresh spawn — ending with N solo matches instead
    // of one populated match.
    //
    // The appended players get invitations together with the original pending
    // players when INSTANCE_READY arrives (IpcServer's MSG_INSTANCE_READY
    // handler iterates session_guids unconditionally). Each rule owns its own
    // task-force policy (coop rules pin everyone to one team; PvP balances) —
    // we honor result->task_force_assignments verbatim here, never override.
    if (!result->existing_instance_id) {
        std::optional<int64_t> pending_iid;
        for (const auto& [iid, pm] : pending_matches_) {
            if (pm.queue_id != queue_id) continue;
            // Belt-and-suspenders: skip pending entries whose instance has
            // died. Normally DiscardPendingMatchForDeadInstance has already
            // erased these, but guard against the race where MarkStopped
            // ran from a path that didn't call discard.
            if (!IsInstanceActive(iid)) {
                Logger::Log("matchmaking",
                    "[Matchmaking] Queue %u: skipping stale pending instance %lld (STOPPED) during coalesce\n",
                    queue_id, (long long)iid);
                continue;
            }
            if (pm.cap > 0 && pm.session_guids.size() >= pm.cap) {
                Logger::Log("queue-pop",
                    "[Matchmaking] Coalesce skipped pending=%lld (at cap %zu/%u) — looking for next\n",
                    (long long)iid, pm.session_guids.size(), pm.cap);
                continue;
            }
            pending_iid = iid; break;
        }
        if (pending_iid) {
            auto& pm = pending_matches_[*pending_iid];

            int team1 = 0, team2 = 0;
            for (const auto& [_, tf] : pm.task_force_assignments) {
                if (tf == 1) team1++; else team2++;
            }

            int appended = 0;
            size_t guid_idx = 0;
            for (; guid_idx < result->session_guids.size(); ++guid_idx) {
                const auto& guid = result->session_guids[guid_idx];
                if (pm.task_force_assignments.count(guid)) continue;  // defensive
                if (pm.cap > 0 && pm.session_guids.size() >= pm.cap) {
                    Logger::Log("queue-pop",
                        "[Matchmaking] Coalesce stopped at cap pending=%lld (size=%zu cap=%u) — remaining %zu player(s) will spawn fresh on next pop cycle\n",
                        (long long)*pending_iid, pm.session_guids.size(), pm.cap,
                        result->session_guids.size() - guid_idx);
                    break;
                }
                auto tfit = result->task_force_assignments.find(guid);
                const int tf = (tfit != result->task_force_assignments.end()) ? tfit->second : 1;
                pm.session_guids.push_back(guid);
                pm.task_force_assignments[guid] = tf;
                auto pfit = result->profile_ids.find(guid);
                pm.profile_ids[guid] = (pfit != result->profile_ids.end()) ? pfit->second : 0;
                if (tf == 1) team1++; else team2++;
                appended++;
            }

            // Remove ONLY the appended players from the queue. Cap-blocked
            // leftovers stay queued; the next TryPop cycle spawns them fresh.
            for (size_t i = 0; i < guid_idx; ++i) {
                const auto& guid = result->session_guids[i];
                auto& players = queue.players;
                players.erase(
                    std::remove_if(players.begin(), players.end(),
                        [&](const QueuedPlayer& p) { return p.session_guid == guid; }),
                    players.end());
            }

            Logger::Log("matchmaking",
                "[Matchmaking] Queue %u: coalesced %d player(s) into pending instance %lld "
                "(teams now %d/%d, total %zu) — no new spawn\n",
                queue_id, appended, (long long)*pending_iid, team1, team2, pm.session_guids.size());

            // Do NOT call on_match_pop_ — the spawned instance already exists
            // and its MATCH_INVITATIONs will go out when INSTANCE_READY fires.
            return;
        }
    }

    // Remove matched players from queue
    for (const auto& guid : result->session_guids) {
        auto& players = queue.players;
        players.erase(
            std::remove_if(players.begin(), players.end(),
                [&](const QueuedPlayer& p) { return p.session_guid == guid; }),
            players.end());
    }

    Logger::Log("matchmaking", "[Matchmaking] Queue %u popped: %zu players, map=%s mode=%s\n",
        queue_id, result->session_guids.size(), result->map_name.c_str(), result->game_mode.c_str());

    if (on_match_pop_) {
        on_match_pop_(queue_id, std::move(*result));
    }
}
