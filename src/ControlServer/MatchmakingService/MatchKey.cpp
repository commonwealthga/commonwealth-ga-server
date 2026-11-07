#include "src/ControlServer/MatchmakingService/MatchKey.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <map>
#include <mutex>
#include <unordered_map>

namespace MatchKey {
namespace {
struct Entry { int64_t key = 0; int hold_secs = 0; std::chrono::steady_clock::time_point at{}; };
std::mutex g_mu;
std::unordered_map<std::string, Entry> g_keys;
std::unordered_map<int64_t, int64_t> g_instances;

// seconds of hold left: -1 = not holding, INT_MAX-ish = forever
long HoldLeft(const Entry& e) {
    if (e.key == 0 || e.hold_secs == 0) return -1;
    if (e.hold_secs < 0) return 1L << 30;
    const auto used = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - e.at).count();
    return used < e.hold_secs ? (long)(e.hold_secs - used) : -1;
}
bool Released(const Entry& e) { return e.hold_secs != 0 && HoldLeft(e) < 0; }

// Raw key of a party (leader first) + whether it is still holding / its hold ran out.
int64_t PartyKey(const QueuedParty& p, bool* holding, bool* expired) {
    std::lock_guard<std::mutex> lock(g_mu);
    *holding = false; *expired = false;
    auto look = [&](const std::string& g) -> int64_t {
        auto it = g_keys.find(g);
        if (it == g_keys.end() || it->second.key == 0) return 0;
        *expired = Released(it->second);
        *holding = !*expired && HoldLeft(it->second) >= 0;
        return it->second.key;
    };
    if (int64_t k = look(p.leader_guid)) return k;
    for (const auto& m : p.members) if (int64_t k = look(m.session_guid)) return k;
    return 0;
}
}  // namespace

void Set(const std::string& guid, int64_t key, int hold_secs) {
    std::lock_guard<std::mutex> lock(g_mu);
    if (key == 0) { g_keys.erase(guid); return; }
    g_keys[guid] = Entry{key, hold_secs, std::chrono::steady_clock::now()};
}

int64_t KeyOf(const std::string& guid) {
    std::lock_guard<std::mutex> lock(g_mu);
    auto it = g_keys.find(guid);
    if (it == g_keys.end() || Released(it->second)) return 0;
    return it->second.key;
}

bool Holding(const std::string& guid) {
    std::lock_guard<std::mutex> lock(g_mu);
    auto it = g_keys.find(guid);
    return it != g_keys.end() && HoldLeft(it->second) >= 0;
}

int SecondsToNextRelease(const std::vector<QueuedParty>& parties) {
    std::lock_guard<std::mutex> lock(g_mu);
    long best = -1;
    for (const auto& p : parties)
        for (const auto& m : p.members) {
            auto it = g_keys.find(m.session_guid);
            if (it == g_keys.end()) continue;
            const long left = HoldLeft(it->second);
            if (left >= 0 && left < (1L << 30) && (best < 0 || left < best)) best = left;
        }
    return (int)best;
}

void NoteInstance(int64_t instance_id, const std::vector<std::string>& guids) {
    for (const auto& g : guids)
        if (int64_t k = KeyOf(g)) {
            std::lock_guard<std::mutex> lock(g_mu);
            g_instances[instance_id] = k;
            Logger::Log("matchmaking", "[MatchKey] instance %lld key %lld\n", (long long)instance_id, (long long)k);
            return;
        }
}

int64_t InstanceKey(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(g_mu);
    auto it = g_instances.find(instance_id);
    return it == g_instances.end() ? 0 : it->second;
}

std::vector<Group> Groups(const std::vector<QueuedParty>& eligible,
                          const std::vector<RunningInstance>& instances,
                          const std::set<int64_t>& pending_keys) {
    struct PK { const QueuedParty* p; int64_t key; bool holding; bool expired; };
    std::vector<PK> pk;
    std::set<int64_t> live = pending_keys;  // keys with an attacker queued or a match going
    for (const auto& ri : instances) if (int64_t k = InstanceKey(ri.instance_id)) live.insert(k);
    for (const auto& p : eligible) {
        PK e{&p, 0, false, false};
        e.key = PartyKey(p, &e.holding, &e.expired);
        if (e.key && !e.holding && !e.expired) live.insert(e.key);
        pk.push_back(e);
    }
    std::vector<Group> out;
    std::map<int64_t, size_t> at;
    for (auto& e : pk) {
        // hold ran out: stay with the territory while its attack is queued or running, else match like anyone
        const int64_t k = (e.expired && !live.count(e.key)) ? 0 : e.key;
        const bool holding = e.holding;
        auto it = at.find(k);
        if (it == at.end()) {
            Group g; g.key = k; g.hold_only = true;
            for (const auto& ri : instances)
                if (k == 0 || InstanceKey(ri.instance_id) == k) g.instances.push_back(ri);
            it = at.emplace(k, out.size()).first;
            out.push_back(std::move(g));
        }
        Group& g = out[it->second];
        g.parties.push_back(*e.p);
        if (!holding) g.hold_only = false;
    }
    std::vector<Group> kept;
    for (auto& g : out) {
        if (g.hold_only && g.instances.empty() && !pending_keys.count(g.key)) continue;
        kept.push_back(std::move(g));
    }
    std::stable_sort(kept.begin(), kept.end(), [](const Group& a, const Group& b) {
        auto first = [](const Group& g) {
            auto t = g.parties.front().joined_at;
            for (const auto& p : g.parties) t = std::min(t, p.joined_at);
            return t;
        };
        return first(a) < first(b);
    });
    return kept;
}

int SettingInt(const char* key, int def) {
    sqlite3* db = Database::GetConnection();
    if (!db) return def;
    int v = def;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(s, 1, key, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_text(s, 0))
            v = atoi((const char*)sqlite3_column_text(s, 0));
        sqlite3_finalize(s);
    }
    return v;
}

}  // namespace MatchKey
