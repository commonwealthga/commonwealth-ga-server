#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/MatchmakingService/RuleFactory.hpp"
#include "src/ControlServer/MatchmakingService/SidePlacement.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <limits>
#include <random>

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------

std::unordered_map<uint32_t, MatchmakingService::Queue> MatchmakingService::queues_;
MatchmakingService::MatchPopCallback MatchmakingService::on_match_pop_;
MatchmakingService::InstanceProvider MatchmakingService::instance_provider_;
std::unordered_map<int64_t, PendingMatch> MatchmakingService::pending_matches_;
std::unordered_map<int64_t, PendingMatch> MatchmakingService::ready_match_reservations_;
std::unordered_map<int64_t, std::unordered_map<std::string, int>>
    MatchmakingService::pre_assigned_teams_;
asio::io_context* MatchmakingService::io_ctx_ = nullptr;

// ---------------------------------------------------------------------------
// Init / reload
// ---------------------------------------------------------------------------

static std::mt19937& MatchRng() {
    static std::mt19937 eng{std::random_device{}()};
    return eng;
}

// Coop (DataDriven) queues join + reserve READY instances; the other rules
// (DoubleAgent, VersusSides) always spawn fresh, so they never reserve.
static bool TracksReadyReservations(const QueueConfig& cfg) {
    return cfg.rule_class.empty()
        || cfg.rule_class == "DataDriven"
        || cfg.rule_class == "Coop";
}

static TaskforcePolicy ParseTaskforcePolicyLogged(const std::string& s, uint32_t qid) {
    bool ok = true;
    auto v = mm::ParseTaskforcePolicy(s, &ok);
    if (!ok) Logger::Log("matchmaking",
        "[Matchmaking] Queue %u unknown taskforce_policy '%s' — defaulting pinned_1\n", qid, s.c_str());
    return v;
}
static TeamPolicy ParseTeamPolicyLogged(const std::string& s, uint32_t qid) {
    bool ok = true;
    auto v = mm::ParseTeamPolicy(s, &ok);
    if (!ok) Logger::Log("matchmaking",
        "[Matchmaking] Queue %u unknown team_policy '%s' — defaulting mixed\n", qid, s.c_str());
    return v;
}
static TeamSidePolicy ParseTeamSidePolicyLogged(const std::string& s, uint32_t qid) {
    bool ok = true;
    auto v = mm::ParseTeamSidePolicy(s, &ok);
    if (!ok) Logger::Log("matchmaking",
        "[Matchmaking] Queue %u unknown team_side_policy '%s' — defaulting ignore\n", qid, s.c_str());
    return v;
}
static PopDelayPolicy ParsePopDelayPolicyLogged(const char* raw, uint32_t qid) {
    bool ok = true;
    auto v = mm::ParsePopDelayPolicy(raw ? raw : "", &ok);
    if (!ok) Logger::Log("matchmaking",
        "[Matchmaking] Queue %u unknown pop_delay_policy '%s' — defaulting halve_on_join\n",
        qid, raw ? raw : "");
    return v;
}

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
        "       min_players_to_pop, max_players_per_instance, pop_delay_seconds,"
        "       pop_delay_policy, instant_pop_when_full,"
        "       marshal_difficulty_value_id, requires_pvp_verification,"
        "       team_policy, team_side_policy, max_team_size "
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
                        ? 0u : (uint32_t)sqlite3_column_int(stmt, col);
        col++;
        if (auto* p = sqlite3_column_text(stmt, col++)) c.name = (const char*)p;
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL)
            c.rule_class = (const char*)sqlite3_column_text(stmt, col);
        col++;
        c.taskforce_policy = ParseTaskforcePolicyLogged(
            sqlite3_column_text(stmt, col) ? (const char*)sqlite3_column_text(stmt, col) : "",
            c.queue_id);
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
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL)
            c.remaining_seconds = (uint32_t)sqlite3_column_int(stmt, col);
        col++;
        c.min_players_to_pop = (uint32_t)sqlite3_column_int(stmt, col++);
        if (c.min_players_to_pop == 0) {
            Logger::Log("matchmaking",
                "[Matchmaking] Queue %u min_players_to_pop=0 invalid — clamping to 1\n", c.queue_id);
            c.min_players_to_pop = 1;
        }
        c.max_players_per_instance = (uint32_t)sqlite3_column_int(stmt, col++);
        c.pop_delay_seconds        = (float)sqlite3_column_double(stmt, col++);
        c.pop_delay_policy = ParsePopDelayPolicyLogged(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, col++)), c.queue_id);
        c.instant_pop_when_full = sqlite3_column_int(stmt, col++) != 0;
        if (sqlite3_column_type(stmt, col) != SQLITE_NULL)
            c.marshal_difficulty_value_id = (uint32_t)sqlite3_column_int(stmt, col);
        col++;
        c.requires_pvp_verification = sqlite3_column_int(stmt, col++) != 0;
        c.team_policy = ParseTeamPolicyLogged(
            sqlite3_column_text(stmt, col) ? (const char*)sqlite3_column_text(stmt, col) : "mixed",
            c.queue_id);
        col++;
        c.team_side_policy = ParseTeamSidePolicyLogged(
            sqlite3_column_text(stmt, col) ? (const char*)sqlite3_column_text(stmt, col) : "ignore",
            c.queue_id);
        col++;
        c.max_team_size = (uint32_t)sqlite3_column_int(stmt, col++);
        out.push_back(std::move(c));
    }
    sqlite3_finalize(stmt);

    for (auto& c : out) {
        if (c.map_pool_id == 0) continue;
        sqlite3_stmt* ps = nullptr;
        const char* psql =
            "SELECT map_name, game_mode, weight, min_players, max_players "
            "FROM ga_map_pool_entries WHERE map_pool_id = ? AND enabled = 1";
        if (sqlite3_prepare_v2(db, psql, -1, &ps, nullptr) != SQLITE_OK || !ps) continue;
        sqlite3_bind_int(ps, 1, (int)c.map_pool_id);
        while (sqlite3_step(ps) == SQLITE_ROW) {
            MapModeEntry m;
            if (auto* p = sqlite3_column_text(ps, 0)) m.map_name  = (const char*)p;
            if (auto* p = sqlite3_column_text(ps, 1)) m.game_mode = (const char*)p;
            m.weight = std::max(1, sqlite3_column_int(ps, 2));
            if (sqlite3_column_type(ps, 3) != SQLITE_NULL) m.min_players = sqlite3_column_int(ps, 3);
            if (sqlite3_column_type(ps, 4) != SQLITE_NULL) m.max_players = sqlite3_column_int(ps, 4);
            c.map_pool.push_back(std::move(m));
        }
        sqlite3_finalize(ps);
    }
    return out;
}

void MatchmakingService::Init() {
    queues_.clear();
    pending_matches_.clear();
    ready_match_reservations_.clear();
    pre_assigned_teams_.clear();
    on_match_pop_ = nullptr;
    instance_provider_ = nullptr;
    io_ctx_ = nullptr;
    Logger::Log("matchmaking", "[Matchmaking] Initialized — loading queue configs from DB\n");

    for (auto& cfg : LoadAllQueueConfigsFromDb()) {
        const uint32_t qid = cfg.queue_id;
        queues_[qid] = BuildQueue(std::move(cfg));
        Logger::Log("matchmaking",
            "[Matchmaking] Loaded queue %u '%s' (rule=%s team_policy=%d side=%d pool=%zu)\n",
            qid, queues_[qid].config.name.c_str(),
            queues_[qid].config.rule_class.empty() ? "Coop" : queues_[qid].config.rule_class.c_str(),
            (int)queues_[qid].config.team_policy, (int)queues_[qid].config.team_side_policy,
            queues_[qid].config.map_pool.size());
    }
}

void MatchmakingService::ReloadQueues() {
    auto fresh = LoadAllQueueConfigsFromDb();

    std::unordered_map<uint32_t, std::vector<QueuedParty>> kept_parties;
    std::unordered_map<uint32_t, std::optional<DelayedPop>> kept_delays;
    for (auto& [qid, q] : queues_) {
        kept_parties[qid] = std::move(q.parties);
        kept_delays[qid]  = std::move(q.delayed_pop);
    }

    std::unordered_map<uint32_t, Queue> rebuilt;
    rebuilt.reserve(fresh.size());
    for (auto& cfg : fresh) {
        const uint32_t qid = cfg.queue_id;
        Queue q = BuildQueue(std::move(cfg));
        auto pit = kept_parties.find(qid);
        if (pit != kept_parties.end()) { q.parties = std::move(pit->second); kept_parties.erase(pit); }
        auto dit = kept_delays.find(qid);
        if (dit != kept_delays.end()) { q.delayed_pop = std::move(dit->second); kept_delays.erase(dit); }
        rebuilt[qid] = std::move(q);
    }

    for (const auto& [qid, parties] : kept_parties) {
        if (parties.empty()) continue;
        Logger::Log("matchmaking",
            "[Matchmaking] ReloadQueues: queue %u removed — dropped %zu queued party(ies)\n",
            qid, parties.size());
    }
    for (auto& [qid, dp] : kept_delays) {
        if (dp && dp->timer) {
            dp->timer->cancel();
            Logger::Log("queue-pop",
                "[Matchmaking] DelayedPop cancelled queue=%u reason=queue_removed (ReloadQueues)\n", qid);
        }
    }

    queues_ = std::move(rebuilt);
    Logger::Log("matchmaking", "[Matchmaking] ReloadQueues: %zu queue(s) now active\n", queues_.size());
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

size_t MatchmakingService::QueuedPlayerCount(const Queue& q) {
    size_t n = 0;
    for (const auto& p : q.parties) n += p.members.size();
    return n;
}

bool MatchmakingService::GetContinueInQueue(uint32_t queue_id) {
    auto it = queues_.find(queue_id);
    return it != queues_.end() && it->second.config.continue_in_queue;
}

std::vector<QueueConfig> MatchmakingService::GetEnabledQueueConfigs() {
    std::vector<QueueConfig> out;
    out.reserve(queues_.size());
    for (const auto& [qid, q] : queues_)
        if (q.config.enabled) out.push_back(q.config);
    std::sort(out.begin(), out.end(), [](const QueueConfig& a, const QueueConfig& b) {
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

uint32_t MatchmakingService::GetQueueInstanceCap(const QueueConfig& cfg, uint32_t instance_max_players) {
    return mm::QueueInstanceCap(cfg, instance_max_players);
}

MatchmakingService::QueuedProfileCounts
MatchmakingService::GetQueuedProfileCounts(uint32_t queue_id) {
    QueuedProfileCounts counts{0, 0, 0, 0};
    auto tally = [&](uint32_t profile_id) {
        switch (profile_id) {
            case 680: counts.assault++;  break;
            case 567: counts.medic++;    break;
            case 681: counts.recon++;    break;
            case 679: counts.robotics++; break;
            default: break;
        }
    };
    auto it = queues_.find(queue_id);
    if (it != queues_.end()) {
        const auto& cfg = it->second.config;
        // Queued parties destined for a PARTY_LOCKED (private) match are
        // excluded — a solo who queues can't be matched with them. That's a
        // team in an own_match queue, or any party in a versus_sides (1v1)
        // queue (every such match is private).
        for (const auto& party : it->second.parties) {
            const bool destined_private =
                cfg.team_policy == TeamPolicy::VersusSides
                || (cfg.team_policy == TeamPolicy::OwnMatch && party.is_team);
            if (destined_private) continue;
            for (const auto& m : party.members) tally(m.profile_id);
        }
    }
    // Popped-but-not-yet-registered players: skip PARTY_LOCKED matches.
    for (const auto& [iid, pm] : pending_matches_) {
        if (pm.queue_id != queue_id) continue;
        if (pm.access_mode == AccessMode::PartyLocked) continue;
        for (const auto& [guid, pid] : pm.profile_ids) tally(pid);
    }
    for (const auto& [iid, ready] : ready_match_reservations_) {
        if (ready.queue_id != queue_id) continue;
        if (ready.access_mode == AccessMode::PartyLocked) continue;
        for (const auto& [guid, pid] : ready.profile_ids) tally(pid);
    }
    return counts;
}

std::optional<uint32_t>
MatchmakingService::GetDelayedPopRemainingSeconds(uint32_t queue_id) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end() || !it->second.delayed_pop) return std::nullopt;
    auto delta = it->second.delayed_pop->fires_at - std::chrono::steady_clock::now();
    if (delta <= std::chrono::milliseconds(0)) return std::nullopt;
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
    std::vector<const MapModeEntry*> candidates;
    for (const auto& e : pool) if (matches(e)) candidates.push_back(&e);

    bool nearest_fit_used = false;
    int  best_distance    = 0;
    if (candidates.empty()) {
        nearest_fit_used = true;
        auto window_distance = [count](const MapModeEntry& e) -> int {
            if (e.min_players && count < *e.min_players) return *e.min_players - count;
            if (e.max_players && count > *e.max_players) return count - *e.max_players;
            return 0;
        };
        best_distance = std::numeric_limits<int>::max();
        for (const auto& e : pool) {
            const int d = window_distance(e);
            if (d < best_distance) { best_distance = d; candidates.clear(); candidates.push_back(&e); }
            else if (d == best_distance) candidates.push_back(&e);
        }
    }
    if (candidates.empty()) return std::nullopt;

    int total = 0;
    for (const auto* e : candidates) total += e->weight;
    std::uniform_int_distribution<int> dist(0, total - 1);
    int roll = dist(MatchRng());
    const MapModeEntry* picked = candidates.back();
    for (const auto* e : candidates) { roll -= e->weight; if (roll < 0) { picked = e; break; } }

    Logger::Log("queue-pop",
        "[Matchmaking] map_pool %s queue=%u count=%d candidates=%zu/%zu picked=%s\n",
        nearest_fit_used ? "nearest" : "filter", queue_id, count,
        candidates.size(), pool.size(), picked->map_name.c_str());
    return MapModeSpec{ picked->map_name, picked->game_mode };
}

void MatchmakingService::SetMatchPopCallback(MatchPopCallback cb) { on_match_pop_ = std::move(cb); }
void MatchmakingService::SetInstanceProvider(InstanceProvider provider) { instance_provider_ = std::move(provider); }
void MatchmakingService::SetIoContext(asio::io_context* io) { io_ctx_ = io; }

// ---------------------------------------------------------------------------
// Party / player management
// ---------------------------------------------------------------------------

uint64_t MatchmakingService::SoloPartyId(const std::string& session_guid) {
    // Top bit set so it never collides with TeamService team ids (small ints).
    return 0x8000000000000000ull | (uint64_t)(std::hash<std::string>{}(session_guid) & 0x7FFFFFFFu);
}

void MatchmakingService::AddParty(uint32_t queue_id, const QueuedParty& party) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) {
        Logger::Log("matchmaking", "[Matchmaking] AddParty: unknown queue %u\n", queue_id);
        return;
    }
    if (party.members.empty()) return;

    // De-dup: a re-queue of the same party replaces the old entry.
    auto& parties = it->second.parties;
    parties.erase(std::remove_if(parties.begin(), parties.end(),
        [&](const QueuedParty& p) { return p.party_id == party.party_id; }), parties.end());
    parties.push_back(party);

    Logger::Log("matchmaking",
        "[Matchmaking] Party %llu (%s, %zu member(s)) joined queue %u (%zu player(s) queued)\n",
        (unsigned long long)party.party_id, party.is_team ? "team" : "solo",
        party.members.size(), queue_id, QueuedPlayerCount(it->second));

    OnQueueChanged(queue_id, "join", party.leader_guid);
}

void MatchmakingService::AddPlayer(uint32_t queue_id, const QueuedPlayer& player) {
    QueuedParty party;
    party.party_id    = SoloPartyId(player.session_guid);
    party.is_team     = false;
    party.leader_guid = player.session_guid;
    party.joined_at   = player.joined_at;
    party.members.push_back(player);
    AddParty(queue_id, party);
}

// Shared post-mutation handling: instant-pop-when-full, delay re-arm, TryPop.
void MatchmakingService::OnQueueChanged(uint32_t queue_id, const char* trigger,
                                        const std::string& who) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) return;
    auto& q = it->second;
    const size_t players = QueuedPlayerCount(q);

    const bool is_join = std::string(trigger) == "join";
    if (is_join && q.config.instant_pop_when_full
            && q.config.max_players_per_instance > 0
            && players >= q.config.max_players_per_instance) {
        Logger::Log("queue-pop",
            "[Matchmaking] InstantPop queue=%u (players=%zu >= max=%u)\n",
            queue_id, players, q.config.max_players_per_instance);
        if (q.delayed_pop) {
            if (q.delayed_pop->timer) q.delayed_pop->timer->cancel();
            q.delayed_pop.reset();
        }
        TryPop(queue_id, /*delay_elapsed=*/true);
        return;
    }

    if (q.delayed_pop) MaybeResetDelayedPop(q, trigger, who);
    else               TryPop(queue_id);
}

bool MatchmakingService::RemoveParty(uint64_t party_id) {
    for (auto& [queue_id, q] : queues_) {
        auto& parties = q.parties;
        auto it = std::find_if(parties.begin(), parties.end(),
            [&](const QueuedParty& p) { return p.party_id == party_id; });
        if (it == parties.end()) continue;
        Logger::Log("matchmaking",
            "[Matchmaking] Party %llu removed from queue %u\n",
            (unsigned long long)party_id, queue_id);
        parties.erase(it);
        if (q.delayed_pop) MaybeResetDelayedPop(q, "leave", "");
        return true;
    }
    return false;
}

void MatchmakingService::RemovePlayer(const std::string& session_guid) {
    // 1. Remove the whole party containing this guid from its queue (atomic).
    for (auto& [queue_id, q] : queues_) {
        auto& parties = q.parties;
        auto it = std::find_if(parties.begin(), parties.end(),
            [&](const QueuedParty& p) {
                for (const auto& m : p.members)
                    if (m.session_guid == session_guid) return true;
                return false;
            });
        if (it != parties.end()) {
            Logger::Log("matchmaking",
                "[Matchmaking] Player %s removed from queue %u (party %llu, %zu member(s))\n",
                session_guid.c_str(), queue_id, (unsigned long long)it->party_id,
                it->members.size());
            parties.erase(it);
            if (q.delayed_pop) MaybeResetDelayedPop(q, "leave", session_guid);
            break;
        }
    }

    // 2. Scrub the guid (individually) from any pending match.
    for (auto& [iid, pm] : pending_matches_) {
        auto sit = std::find(pm.session_guids.begin(), pm.session_guids.end(), session_guid);
        if (sit == pm.session_guids.end()) continue;
        pm.session_guids.erase(sit);
        pm.task_force_assignments.erase(session_guid);
        pm.profile_ids.erase(session_guid);
        Logger::Log("matchmaking",
            "[Matchmaking] Player %s removed from pending match for instance %lld (%zu left)\n",
            session_guid.c_str(), (long long)iid, pm.session_guids.size());
        break;
    }

    // 3. Ready reservations.
    RemoveReadyMatchReservation(session_guid, "player removed from matchmaking");
}

// ---------------------------------------------------------------------------
// Pending / ready reservation bookkeeping
// ---------------------------------------------------------------------------

void MatchmakingService::AddPendingMatch(int64_t instance_id, PendingMatch match) {
    Logger::Log("matchmaking",
        "[Matchmaking] Pending match added for instance %lld (%zu players, access=%s)\n",
        (long long)instance_id, match.session_guids.size(), mm::AccessModeToString(match.access_mode));
    pending_matches_[instance_id] = std::move(match);
}

std::optional<PendingMatch> MatchmakingService::ConsumePendingMatch(int64_t instance_id) {
    auto it = pending_matches_.find(instance_id);
    if (it == pending_matches_.end()) return std::nullopt;
    PendingMatch match = std::move(it->second);
    pending_matches_.erase(it);
    return match;
}

void MatchmakingService::TrackReadyMatchReservations(
    int64_t instance_id, uint32_t queue_id, const std::string& game_mode,
    const std::vector<std::string>& session_guids,
    const std::unordered_map<std::string, int>& task_force_assignments,
    const std::unordered_map<std::string, uint32_t>& profile_ids, uint32_t cap) {
    if (instance_id == 0 || session_guids.empty()) return;
    auto qit = queues_.find(queue_id);
    if (qit == queues_.end() || !TracksReadyReservations(qit->second.config)) return;

    auto& ready = ready_match_reservations_[instance_id];
    if (ready.instance_id == 0) {
        ready.instance_id = instance_id;
        ready.queue_id = queue_id;
        ready.game_mode = game_mode;
        ready.cap = cap;
        // Carry access from the instance row so GetQueuedProfileCounts can skip
        // PARTY_LOCKED (private) reservations in the per-class card.
        if (auto inst = InstanceRegistry::GetInstanceById(instance_id))
            ready.access_mode = mm::ParseAccessMode(inst->access_mode);
    }
    if (ready.cap == 0 && cap != 0) ready.cap = cap;

    size_t added = 0;
    for (const auto& guid : session_guids) {
        if (std::find(ready.session_guids.begin(), ready.session_guids.end(), guid)
                != ready.session_guids.end()) continue;
        ready.session_guids.push_back(guid);
        auto tfit = task_force_assignments.find(guid);
        ready.task_force_assignments[guid] = (tfit != task_force_assignments.end()) ? tfit->second : 1;
        auto pfit = profile_ids.find(guid);
        ready.profile_ids[guid] = (pfit != profile_ids.end()) ? pfit->second : 0;
        added++;
    }
    if (added > 0)
        Logger::Log("matchmaking",
            "[Matchmaking] Ready reservations: instance=%lld queue=%u added=%zu total=%zu cap=%u\n",
            (long long)instance_id, queue_id, added, ready.session_guids.size(), ready.cap);
}

std::vector<RunningInstance> MatchmakingService::GetReservedReadyInstances(uint32_t queue_id) {
    std::vector<RunningInstance> out;
    auto qit = queues_.find(queue_id);
    if (qit == queues_.end() || !TracksReadyReservations(qit->second.config)) return out;

    std::vector<int64_t> stale;
    for (const auto& [iid, ready] : ready_match_reservations_) {
        if (ready.queue_id != queue_id || ready.session_guids.empty()) continue;
        auto inst = InstanceRegistry::GetInstanceById(iid);
        if (!inst || inst->state != "READY" || inst->is_home_map || inst->end_mission_at != 0) {
            stale.push_back(iid);
            continue;
        }
        RunningInstance ri;
        ri.instance_id  = inst->instance_id;
        ri.map_name     = inst->map_name;
        ri.game_mode    = inst->game_mode;
        ri.queue_id     = inst->queue_id;
        // Carry the instance's access so a not-yet-populated PARTY_LOCKED /
        // SEALED reserved instance is never presented to the provider as OPEN.
        ri.access_mode  = mm::ParseAccessMode(inst->access_mode);
        {
            const std::string& csv = inst->owner_party_ids;
            size_t i = 0;
            while (i < csv.size()) {
                size_t j = csv.find(',', i);
                if (j == std::string::npos) j = csv.size();
                if (j > i) ri.owner_party_ids.push_back(strtoull(csv.substr(i, j - i).c_str(), nullptr, 10));
                i = j + 1;
            }
        }
        // Reserved contribution ONLY — the provider adds the live roster.
        for (const auto& [guid, tf] : ready.task_force_assignments) {
            uint32_t profile_id = 0;
            auto pit = ready.profile_ids.find(guid);
            if (pit != ready.profile_ids.end()) profile_id = pit->second;
            TeamSeed& seed = (tf == 2) ? ri.team2 : ri.team1;
            seed.size += 1;
            seed.heal_score += SidePlacement::HealValue(profile_id, tf == 2 ? 2 : 1);
            seed.class_counts[profile_id] += 1;
        }
        ri.player_count = (int)ready.session_guids.size();
        out.push_back(std::move(ri));
    }
    for (int64_t iid : stale) DropReadyMatchReservations(iid);
    return out;
}

void MatchmakingService::RemoveReadyMatchReservation(
    int64_t instance_id, const std::string& session_guid, const char* reason) {
    auto it = ready_match_reservations_.find(instance_id);
    if (it == ready_match_reservations_.end()) return;
    auto& ready = it->second;
    auto sit = std::find(ready.session_guids.begin(), ready.session_guids.end(), session_guid);
    if (sit == ready.session_guids.end()) return;
    ready.session_guids.erase(sit);
    ready.task_force_assignments.erase(session_guid);
    ready.profile_ids.erase(session_guid);
    Logger::Log("matchmaking",
        "[Matchmaking] Ready reservation removed: instance=%lld guid=%s remaining=%zu reason=%s\n",
        (long long)instance_id, session_guid.c_str(), ready.session_guids.size(),
        reason ? reason : "unspecified");
    if (ready.session_guids.empty()) ready_match_reservations_.erase(it);
}

void MatchmakingService::RemoveReadyMatchReservation(
    const std::string& session_guid, const char* reason) {
    for (auto it = ready_match_reservations_.begin(); it != ready_match_reservations_.end(); ++it) {
        auto sit = std::find(it->second.session_guids.begin(),
            it->second.session_guids.end(), session_guid);
        if (sit == it->second.session_guids.end()) continue;
        RemoveReadyMatchReservation(it->first, session_guid, reason);
        return;
    }
}

void MatchmakingService::DropReadyMatchReservations(int64_t instance_id) {
    auto it = ready_match_reservations_.find(instance_id);
    if (it == ready_match_reservations_.end()) return;
    size_t n = it->second.session_guids.size();
    ready_match_reservations_.erase(it);
    if (n > 0)
        Logger::Log("matchmaking",
            "[Matchmaking] Ready reservations dropped: instance=%lld n=%zu\n", (long long)instance_id, n);
}

void MatchmakingService::SetPreAssignedTeam(int64_t instance_id, const std::string& guid, int tf) {
    pre_assigned_teams_[instance_id][guid] = tf;
    Logger::Log("matchmaking",
        "[Matchmaking] PreAssignedTeam set: instance=%lld guid=%s tf=%d\n",
        (long long)instance_id, guid.c_str(), tf);
}

std::optional<int>
MatchmakingService::ConsumePreAssignedTeam(int64_t instance_id, const std::string& guid) {
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
    if (n > 0)
        Logger::Log("matchmaking",
            "[Matchmaking] PreAssignedTeams dropped: instance=%lld n=%zu\n", (long long)instance_id, n);
}

void MatchmakingService::DiscardPendingMatchForDeadInstance(int64_t instance_id, const char* reason) {
    auto it = pending_matches_.find(instance_id);
    if (it == pending_matches_.end()) return;
    Logger::Log("matchmaking",
        "[Matchmaking] Discarding pending match for dead instance %lld (%zu player(s), reason: %s)\n",
        (long long)instance_id, it->second.session_guids.size(), reason ? reason : "unspecified");
    for (const auto& guid : it->second.session_guids)
        TcpSession::DeliverMatchCancelled(guid, reason);
    pending_matches_.erase(it);
}

static bool IsInstanceActive(int64_t instance_id) {
    auto info = InstanceRegistry::GetInstanceById(instance_id);
    return info && info->state != "STOPPED";
}

// ---------------------------------------------------------------------------
// Core logic
// ---------------------------------------------------------------------------

void MatchmakingService::MaybeResetDelayedPop(Queue& q, const char* trigger, const std::string& guid) {
    if (!q.delayed_pop) return;
    const size_t players = QueuedPlayerCount(q);

    if (players < q.config.min_players_to_pop) {
        if (q.delayed_pop->timer) q.delayed_pop->timer->cancel();
        Logger::Log("queue-pop",
            "[Matchmaking] DelayedPop cancelled queue=%u reason=below_min (players=%zu < min=%u) trigger=%s\n",
            q.config.queue_id, players, q.config.min_players_to_pop, trigger);
        q.delayed_pop.reset();
        return;
    }

    const float prev = q.delayed_pop->next_duration;
    float next = prev;
    const bool is_join = std::string(trigger) == "join";
    switch (q.config.pop_delay_policy) {
        case PopDelayPolicy::HalveOnJoin:
            if (!is_join) return;
            next = std::max(0.5f, prev * 0.5f);
            break;
        case PopDelayPolicy::Fixed:
            return;
        case PopDelayPolicy::ResetOnJoin:
            if (!is_join) return;
            next = q.config.pop_delay_seconds;
            break;
    }
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
        if (!it->second.delayed_pop || it->second.delayed_pop->timer != timer) return;
        it->second.delayed_pop.reset();
        Logger::Log("queue-pop", "[Matchmaking] DelayedPop fired queue=%u — proceeding to spawn\n", qid);
        TryPop(qid, /*delay_elapsed=*/true);
    });
    Logger::Log("queue-pop",
        "[Matchmaking] DelayedPop reset queue=%u prev=%.2fs new=%.2fs players=%zu trigger=%s\n",
        q.config.queue_id, prev, next, players, trigger);
}

// Remove the consumed parties (by id) from a queue.
static void RemoveConsumedParties(std::vector<QueuedParty>& parties,
                                  const std::vector<uint64_t>& consumed) {
    parties.erase(std::remove_if(parties.begin(), parties.end(),
        [&](const QueuedParty& p) {
            return std::find(consumed.begin(), consumed.end(), p.party_id) != consumed.end();
        }), parties.end());
}

void MatchmakingService::TryPop(uint32_t queue_id, bool delay_elapsed) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) return;
    auto& queue = it->second;
    if (!queue.rule || queue.parties.empty()) return;

    // Eligibility: pvp-verification gate. A party is withheld (kept queued, kept
    // on the card) unless EVERY member is verified for queues that require it.
    std::vector<QueuedParty> eligible;
    eligible.reserve(queue.parties.size());
    for (const auto& party : queue.parties) {
        if (queue.config.requires_pvp_verification) {
            bool all_verified = true;
            for (const auto& m : party.members)
                if (!Database::IsUserVerifiedForPvp(m.user_id)) { all_verified = false; break; }
            if (!all_verified) continue;
        }
        eligible.push_back(party);
    }
    if (eligible.empty()) return;

    std::vector<RunningInstance> instances;
    if (instance_provider_) instances = instance_provider_(queue_id);

    auto result = queue.rule->Evaluate(eligible, instances);
    if (!result) return;

    // Min-players gate (spawn-new only).
    if (!result->existing_instance_id
            && result->session_guids.size() < queue.config.min_players_to_pop) {
        Logger::Log("queue-pop",
            "[Matchmaking] TryPop queue=%u below min_players_to_pop (have=%zu need=%u)\n",
            queue_id, result->session_guids.size(), queue.config.min_players_to_pop);
        return;
    }

    // Pop-delay gate (spawn-new only).
    if (!delay_elapsed && !result->existing_instance_id
            && queue.config.pop_delay_seconds > 0.0f && io_ctx_) {
        const auto now = std::chrono::steady_clock::now();
        const auto dur_ms = std::chrono::milliseconds((int64_t)(queue.config.pop_delay_seconds * 1000.0f));
        DelayedPop dp;
        dp.next_duration = queue.config.pop_delay_seconds;
        dp.timer = std::make_shared<asio::steady_timer>(*io_ctx_);
        dp.timer->expires_after(dur_ms);
        dp.fires_at = now + dur_ms;
        const uint32_t qid = queue_id;
        auto timer = dp.timer;
        dp.timer->async_wait([qid, timer](const asio::error_code& ec) {
            if (ec) return;
            auto qit = queues_.find(qid);
            if (qit == queues_.end()) return;
            if (!qit->second.delayed_pop || qit->second.delayed_pop->timer != timer) return;
            qit->second.delayed_pop.reset();
            Logger::Log("queue-pop", "[Matchmaking] DelayedPop fired queue=%u — proceeding to spawn\n", qid);
            TryPop(qid, /*delay_elapsed=*/true);
        });
        Logger::Log("queue-pop",
            "[Matchmaking] DelayedPop started queue=%u duration=%.2fs players=%zu\n",
            queue_id, queue.config.pop_delay_seconds, QueuedPlayerCount(queue));
        queue.delayed_pop = std::move(dp);
        return;
    }

    // Fill map from pool for fresh spawns that left it empty.
    if (!result->existing_instance_id && result->map_name.empty()) {
        auto picked = PickRandomMapPoolEntryForCount(queue_id, (int)result->session_guids.size());
        if (!picked) {
            Logger::Log("matchmaking",
                "[Matchmaking] Queue %u rule wants spawn but map_pool empty — skipping pop\n", queue_id);
            return;
        }
        result->map_name  = picked->map_name;
        result->game_mode = picked->game_mode;
    }

    // Coalesce a fresh OPEN result into an existing OPEN pending (cold-start
    // race avoidance). PARTY_LOCKED / SEALED results never coalesce — they
    // each own a private instance. Parties stay atomic: coalesce only when the
    // WHOLE result fits under the pending's cap.
    if (!result->existing_instance_id && result->access_mode == AccessMode::Open) {
        for (auto& [iid, pm] : pending_matches_) {
            if (pm.queue_id != queue_id) continue;
            if (pm.access_mode != AccessMode::Open) continue;
            if (!IsInstanceActive(iid)) continue;
            const size_t need = result->session_guids.size();
            if (pm.cap > 0 && pm.session_guids.size() + need > pm.cap) continue;  // try next pending

            for (const auto& guid : result->session_guids) {
                if (pm.task_force_assignments.count(guid)) continue;  // defensive
                auto tfit = result->task_force_assignments.find(guid);
                pm.session_guids.push_back(guid);
                pm.task_force_assignments[guid] =
                    (tfit != result->task_force_assignments.end()) ? tfit->second : 1;
                auto pfit = result->profile_ids.find(guid);
                pm.profile_ids[guid] = (pfit != result->profile_ids.end()) ? pfit->second : 0;
            }
            RemoveConsumedParties(queue.parties, result->consumed_party_ids);
            Logger::Log("matchmaking",
                "[Matchmaking] Queue %u: coalesced %zu player(s) into pending instance %lld (total %zu)\n",
                queue_id, need, (long long)iid, pm.session_guids.size());
            if (!queue.parties.empty()) TryPop(queue_id, delay_elapsed);
            return;
        }
    }

    // Commit: remove consumed parties, hand the result to the spawn/route callback.
    RemoveConsumedParties(queue.parties, result->consumed_party_ids);

    Logger::Log("matchmaking",
        "[Matchmaking] Queue %u popped: %zu players map=%s mode=%s access=%s\n",
        queue_id, result->session_guids.size(), result->map_name.c_str(),
        result->game_mode.c_str(), mm::AccessModeToString(result->access_mode));

    if (on_match_pop_) on_match_pop_(queue_id, std::move(*result));
    if (!queue.parties.empty()) TryPop(queue_id, delay_elapsed);
}
