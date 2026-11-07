#include "src/ControlServer/InstanceState/InstanceState.hpp"

#include <ctime>
#include <map>
#include <mutex>
#include <unordered_map>

namespace InstanceState {

namespace {
constexpr int64_t kMaxAgeSeconds = 3600;

struct Entry {
    int64_t updated_at = 0;
    nlohmann::json progress;                   // null until the first snapshot
    std::map<int64_t, nlohmann::json> players; // character_id -> MATCH_STATS
};

std::mutex g_mu;
std::unordered_map<int64_t, Entry> g_state;
std::unordered_map<int64_t, nlohmann::json> g_queued;

// Caller holds g_mu.
Entry& Touch(int64_t instance_id) {
    const int64_t now = (int64_t)std::time(nullptr);
    for (auto it = g_state.begin(); it != g_state.end();) {
        if (it->first != instance_id && now - it->second.updated_at > kMaxAgeSeconds) it = g_state.erase(it);
        else ++it;
    }
    Entry& e = g_state[instance_id];
    e.updated_at = now;
    return e;
}
}

void SetProgress(int64_t instance_id, const nlohmann::json& snapshot) {
    if (instance_id <= 0 || !snapshot.is_object()) return;
    std::lock_guard<std::mutex> lk(g_mu);
    Touch(instance_id).progress = snapshot;
}

void SetPlayerStats(int64_t instance_id, int64_t character_id, const nlohmann::json& stats) {
    if (instance_id <= 0 || character_id <= 0 || !stats.is_object()) return;
    std::lock_guard<std::mutex> lk(g_mu);
    Touch(instance_id).players[character_id] = stats;
}

nlohmann::json Get(int64_t instance_id) {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_state.find(instance_id);
    if (it == g_state.end()) return nullptr;
    nlohmann::json players = nlohmann::json::array();
    for (const auto& [cid, st] : it->second.players) players.push_back(st);
    return {{"instance_id", instance_id}, {"updated_at", it->second.updated_at},
            {"progress", it->second.progress}, {"players", players}};
}

int TaskForceOf(int64_t instance_id, int64_t character_id) {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_state.find(instance_id);
    if (it == g_state.end()) return 0;
    auto p = it->second.players.find(character_id);
    return p == it->second.players.end() ? 0 : p->second.value("task_force", 0);
}

int TaskForceOfUser(int64_t instance_id, int64_t user_id) {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_state.find(instance_id);
    if (it == g_state.end() || user_id <= 0) return 0;
    for (const auto& [cid, st] : it->second.players)
        if (st.value("user_id", (int64_t)0) == user_id) return st.value("task_force", 0);
    return 0;
}

void Forget(int64_t instance_id) {
    std::lock_guard<std::mutex> lk(g_mu);
    g_state.erase(instance_id);
    g_queued.erase(instance_id);
}

void Queue(int64_t instance_id, const nlohmann::json& msg) {
    if (instance_id <= 0 || !msg.is_object()) return;
    std::lock_guard<std::mutex> lk(g_mu);
    g_queued[instance_id] = msg;
}

nlohmann::json TakeQueued(int64_t instance_id) {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_queued.find(instance_id);
    if (it == g_queued.end()) return nlohmann::json();
    nlohmann::json msg = std::move(it->second);
    g_queued.erase(it);
    return msg;
}

} // namespace InstanceState
