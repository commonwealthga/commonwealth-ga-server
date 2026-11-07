#include "src/ControlServer/NodeClaims/NodeClaims.hpp"
#include "src/ControlServer/ExternalInstances/ExternalInstances.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/MmrService/MmrService.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/AchievementService/AchievementService.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <algorithm>
#include <ctime>
#include <mutex>
#include <unordered_map>

namespace NodeClaims {

namespace {

struct State {
    int64_t seq = -1;
    int kills = 0, consumes = 0;
    int64_t first_seen = 0;
    bool ended = false;   // rewards for a mission end are paid once
};

std::mutex g_mu;
std::unordered_map<int64_t, State> g_state;

std::string MapOf(int64_t instance_id) {
    auto inst = InstanceRegistry::GetInstanceById(instance_id);
    return inst ? inst->map_name : std::string();
}

int64_t ElapsedSecs(int64_t instance_id, int64_t first_seen) {
    auto inst = InstanceRegistry::GetInstanceById(instance_id);
    const int64_t start = inst && inst->started_at > 0 ? inst->started_at : first_seen;
    const int64_t now = (int64_t)time(nullptr);
    return start > 0 && now > start ? now - start : 0;
}

// Our side's record of who was in the instance (joined_at / left_at as the
// control server saw them, 0 = still in).
nlohmann::json PlayersOf(int64_t instance_id) {
    nlohmann::json out = nlohmann::json::array();
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (!db || sqlite3_prepare_v2(db,
            "SELECT ip.character_id, COALESCE(c.user_id,0), ip.task_force_number, ip.joined_at, "
            "COALESCE(ip.left_at,0) FROM ga_instance_players ip "
            "LEFT JOIN ga_characters c ON c.id = ip.character_id "
            "WHERE ip.instance_id = ?1 ORDER BY ip.id", -1, &st, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int64(st, 1, instance_id);
    while (sqlite3_step(st) == SQLITE_ROW)
        out.push_back({{"char_id", sqlite3_column_int64(st, 0)}, {"user_id", sqlite3_column_int64(st, 1)},
                       {"task_force", sqlite3_column_int(st, 2)}, {"joined_at", sqlite3_column_int64(st, 3)},
                       {"left_at", sqlite3_column_int64(st, 4)}});
    sqlite3_finalize(st);
    return out;
}

bool BotExists(int bot_id) {
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (!db || sqlite3_prepare_v2(db, "SELECT 1 FROM asm_data_set_bots WHERE bot_id = ?1 LIMIT 1",
            -1, &st, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(st, 1, bot_id);
    const bool ok = sqlite3_step(st) == SQLITE_ROW;
    sqlite3_finalize(st);
    return ok;
}

// Same rule as the instance-side kill award (Database::AwardKillXp in the DLL).
bool ApplyKill(int64_t character_id, int bot_id, int& xp, int& credits) {
    xp = credits = 0;
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (!db || sqlite3_prepare_v2(db,
            "SELECT COALESCE(xp_value,0), COALESCE(currency_value,0) "
            "FROM asm_data_set_bots WHERE bot_id = ?1 LIMIT 1", -1, &st, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(st, 1, bot_id);
    if (sqlite3_step(st) == SQLITE_ROW) {
        xp      = std::max(0, sqlite3_column_int(st, 0));
        credits = std::max(0, sqlite3_column_int(st, 1));
    }
    sqlite3_finalize(st);
    if (xp == 0 && credits == 0) return true;
    if (sqlite3_prepare_v2(db,
            "UPDATE ga_characters SET xp = COALESCE(xp,0) + ?1, credits = COALESCE(credits,0) + ?2 "  // cap = trigger
            ""
            "WHERE id = ?3", -1, &st, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(st, 1, xp);
    sqlite3_bind_int(st, 2, credits);
    sqlite3_bind_int64(st, 3, character_id);
    const bool ok = sqlite3_step(st) == SQLITE_DONE && sqlite3_changes(db) == 1;
    sqlite3_finalize(st);
    return ok;
}

// Same rule as the instance-side Database::ConsumeInvItem.
bool ApplyConsume(int64_t user_id, int64_t inventory_id) {
    sqlite3* db = Database::GetConnection();
    if (!db) return false;
    sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
    bool ok = false;
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM ga_players_inventory WHERE id = ?1 AND user_id = ?2 AND item_id > 0",
            -1, &st, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(st, 1, inventory_id);
        sqlite3_bind_int64(st, 2, user_id);
        ok = sqlite3_step(st) == SQLITE_DONE && sqlite3_changes(db) == 1;
        sqlite3_finalize(st);
    }
    if (ok && sqlite3_prepare_v2(db,
            "DELETE FROM ga_character_devices WHERE inventory_id = ?1 "
            "AND character_id IN (SELECT id FROM ga_characters WHERE user_id = ?2)",
            -1, &st, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(st, 1, inventory_id);
        sqlite3_bind_int64(st, 2, user_id);
        sqlite3_step(st);
        sqlite3_finalize(st);
    }
    sqlite3_exec(db, ok ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
    return ok;
}

} // namespace

bool Handle(int64_t instance_id, const nlohmann::json& j) {
    ExternalInstances::Owner o;
    if (!ExternalInstances::GetOwner(instance_id, o)) {
        Logger::Log("ipc", "[NodeClaim] instance=%lld not external - ignored\n", (long long)instance_id);
        return false;
    }
    const std::string kind = j.value("kind", "");
    const bool kill = kind == "kill";
    const int64_t ref  = j.value(kill ? "bot_id" : "inventory_id", (int64_t)0);
    const int64_t seq  = j.value("seq", (int64_t)-1);
    const double  t    = j.value("t", 0.0);

    State snapshot;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        State& s = g_state[instance_id];
        if (s.first_seen == 0) s.first_seen = (int64_t)time(nullptr);
        snapshot = s;
    }
    const int64_t elapsed = ElapsedSecs(instance_id, snapshot.first_seen);

    const char* why = nullptr;
    if ((kind != "kill" && kind != "consume") || ref <= 0) why = "bad claim";
    else if (j.value("user_id", (int64_t)0) != o.user_id ||
             j.value("character_id", (int64_t)0) != o.character_id) why = "not the owner";

    bool accept = why == nullptr;
    bool builtin = accept;
    if (accept && ServiceHooks::Enabled("node_claim")) {
        nlohmann::json d = {{"user_id", o.user_id}, {"char_id", o.character_id}, {"node_id", o.node_id},
            {"instance_id", instance_id}, {"map", MapOf(instance_id)}, {"kind", kind},
            {"seq", seq}, {"t", t}, {"elapsed_s", elapsed}};
        d[kill ? "bot_id" : "inventory_id"] = ref;
        auto r = ServiceHooks::Call("node_claim", d);
        if (r && r->is_object()) {
            builtin = false;
            accept = r->value("accept", false) == true;
            if (!accept) why = "service rejected";
        }
    }
    if (builtin) {
        if (seq <= snapshot.seq) { accept = false; why = "seq"; }
        else if (kill && !BotExists((int)ref)) { accept = false; why = "unknown bot"; }
        else if (kill && snapshot.kills >= kKillCap) { accept = false; why = "kill cap"; }
        else if (!kill && snapshot.consumes >= kConsumeCap) { accept = false; why = "consume cap"; }
    }
    {
        std::lock_guard<std::mutex> lk(g_mu);
        State& s = g_state[instance_id];
        if (seq > s.seq) s.seq = seq;
        if (accept) (kill ? s.kills : s.consumes)++;
    }
    if (!accept) {
        Logger::Log("ipc", "[NodeClaim] instance=%lld user=%lld %s=%lld seq=%lld REJECTED (%s)\n",
            (long long)instance_id, (long long)o.user_id, kind.c_str(), (long long)ref, (long long)seq, why);
        return false;
    }
    int xp = 0, credits = 0;
    const bool ok = kill ? ApplyKill(o.character_id, (int)ref, xp, credits) : ApplyConsume(o.user_id, ref);
    if (ok && kill) TcpSession::LevelLinesForCharacter(o.character_id);
    Logger::Log("ipc", "[NodeClaim] instance=%lld user=%lld %s=%lld seq=%lld -> %s xp+%d cr+%d\n",
        (long long)instance_id, (long long)o.user_id, kind.c_str(), (long long)ref, (long long)seq,
        ok ? "applied" : "apply failed", xp, credits);
    return ok;
}

bool MissionEndAccepted(int64_t instance_id, bool* defer_results, const nlohmann::json& report) {
    if (defer_results) *defer_results = false;
    ExternalInstances::Owner o;
    if (!ExternalInstances::GetOwner(instance_id, o)) return true;
    State snapshot;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        auto it = g_state.find(instance_id);
        if (it != g_state.end()) snapshot = it->second;
    }
    if (snapshot.ended) {
        Logger::Log("ipc", "[NodeClaim] instance=%lld repeated mission end -> ignored (rewards already paid)\n",
            (long long)instance_id);
        return false;
    }
    const int64_t first_seen = snapshot.first_seen;
    const int64_t elapsed = ElapsedSecs(instance_id, first_seen);
    bool accept = elapsed >= kMinMissionSecs;
    if (ServiceHooks::Enabled("mission_end")) {
        nlohmann::json d = {{"user_id", o.user_id}, {"char_id", o.character_id},
            {"node_id", o.node_id}, {"instance_id", instance_id}, {"map", MapOf(instance_id)},
            {"elapsed_s", elapsed}, {"claimed_kills", snapshot.kills},
            {"claimed_consumes", snapshot.consumes}, {"players", PlayersOf(instance_id)}};
        auto inst = InstanceRegistry::GetInstanceById(instance_id);
        if (inst) d["queue_id"] = inst->queue_id;
        if (report.is_object())
            for (auto it = report.begin(); it != report.end(); ++it)
                if (!d.contains(it.key())) d[it.key()] = it.value();
        auto r = ServiceHooks::Call("mission_end", d);
        if (r && r->is_object()) {
            accept = r->value("accept", false) == true;
            if (accept && defer_results) *defer_results = r->value("defer_results", false) == true;
        }
    }
    if (accept) {
        std::lock_guard<std::mutex> lk(g_mu);
        g_state[instance_id].ended = true;
    }
    Logger::Log("ipc", "[NodeClaim] instance=%lld mission end after %llds -> rewards %s%s\n",
        (long long)instance_id, (long long)elapsed, accept ? "accepted" : "REJECTED",
        defer_results && *defer_results ? " (rating/AvA results deferred)" : "");
    return accept;
}

namespace {
// Binds ?1 instance_id, ?2 a, ?3 now (unused parameters are ignored by SQLite).
bool ExecResultRow(const char* sql, int64_t instance_id, int a = 0, bool want_change = true) {
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (!db || sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(st, 1, instance_id);
    sqlite3_bind_int(st, 2, a);
    sqlite3_bind_int64(st, 3, (int64_t)time(nullptr));
    const bool ok = sqlite3_step(st) == SQLITE_DONE && (!want_change || sqlite3_changes(db) == 1);
    sqlite3_finalize(st);
    return ok;
}
}  // namespace

void DeferResults(int64_t instance_id, int winning_task_force) {
    ExecResultRow("INSERT OR IGNORE INTO ga_instance_result_hold (instance_id, winning_task_force, held_at, state) "
                  "VALUES (?1, ?2, ?3, 'held')", instance_id, winning_task_force, false);
}

bool CommitResults(int64_t instance_id, std::string& message) {
    int wtf = 0;
    {
        sqlite3* db = Database::GetConnection();
        sqlite3_stmt* st = nullptr;
        bool found = false;
        if (db && sqlite3_prepare_v2(db, "SELECT winning_task_force FROM ga_instance_result_hold "
                "WHERE instance_id = ?1 AND state = 'held'", -1, &st, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(st, 1, instance_id);
            if (sqlite3_step(st) == SQLITE_ROW) { found = true; wtf = sqlite3_column_int(st, 0); }
            sqlite3_finalize(st);
        }
        if (!found) { message = "no deferred results for this instance"; return false; }
    }
    if (!ExecResultRow("DELETE FROM ga_instance_result_hold WHERE instance_id = ?1 AND state = 'held'",
                       instance_id)) {
        message = "commit failed (db)";
        return false;
    }
    MmrService::FoldUnprocessed();
    auto inst = InstanceRegistry::GetInstanceById(instance_id);
    int ava_rewards = 0;
    if (inst && inst->queue_id >= 40 && inst->queue_id <= 45) {
        for (const auto& rw : PlayerSessionStore::ResolveAvaMatch(instance_id, wtf)) {
            TcpSession::SendRewardDetails(rw);
            ava_rewards += (int)rw.characters.size();
        }
        std::vector<std::pair<int64_t, int>> unlocks;
        for (const auto& u : AchievementService::CreditAvaMatch(instance_id))
            unlocks.emplace_back(u.character_id, u.achievement_id);
        if (!unlocks.empty()) TcpSession::PushAchievementUnlocks(unlocks);
    }
    Logger::Log("ipc", "[NodeClaim] instance=%lld deferred results committed (wtf=%d, ava rewards=%d)\n",
        (long long)instance_id, wtf, ava_rewards);
    message = "committed instance=" + std::to_string(instance_id);
    return true;
}

bool DropResults(int64_t instance_id, std::string& message) {
    if (!ExecResultRow("UPDATE ga_instance_result_hold SET state = 'dropped', held_at = ?3 "
                       "WHERE instance_id = ?1 AND state = 'held'", instance_id)) {
        message = "no deferred results for this instance";
        return false;
    }
    Logger::Log("ipc", "[NodeClaim] instance=%lld deferred results dropped\n", (long long)instance_id);
    message = "dropped instance=" + std::to_string(instance_id);
    return true;
}

nlohmann::json ListResults() {
    nlohmann::json out = nlohmann::json::array();
    sqlite3* db = Database::GetConnection();
    sqlite3_stmt* st = nullptr;
    if (!db || sqlite3_prepare_v2(db, "SELECT instance_id, winning_task_force, held_at, state "
            "FROM ga_instance_result_hold ORDER BY held_at DESC LIMIT 200", -1, &st, nullptr) != SQLITE_OK)
        return out;
    while (sqlite3_step(st) == SQLITE_ROW)
        out.push_back({{"instance_id", sqlite3_column_int64(st, 0)},
                       {"winning_task_force", sqlite3_column_int(st, 1)},
                       {"at", sqlite3_column_int64(st, 2)},
                       {"state", (const char*)sqlite3_column_text(st, 3)}});
    sqlite3_finalize(st);
    return out;
}

void Forget(int64_t instance_id) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_state.erase(instance_id);
}

} // namespace NodeClaims
