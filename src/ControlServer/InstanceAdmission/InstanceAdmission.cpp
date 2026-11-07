#include "src/ControlServer/InstanceAdmission/InstanceAdmission.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include "src/ControlServer/Logger.hpp"

#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>

namespace {
    using Clock = std::chrono::steady_clock;

    struct Admit { std::string guid; Clock::time_point at; };
    struct Line {
        std::deque<std::string> waiting;
        std::deque<Admit>       recent;    // admitted, row may not exist yet
        Clock::time_point       hook_at{};
        bool                    hook_hold = false;
    };

    // Admitted players get their instance row once PLAYER_REGISTER acks
    // (a few seconds); count them as present until then.
    constexpr auto kReserve   = std::chrono::seconds(30);
    // ServiceHooks answers are reused for this long per map.
    constexpr auto kHookReuse = std::chrono::seconds(5);

    std::mutex                  g_mutex;
    std::map<std::string, Line> g_lines;

    // Recent admits whose row is not there yet still take a slot.
    int PendingAdmits(Line& line, int64_t instance_id) {
        const auto now = Clock::now();
        while (!line.recent.empty() && now - line.recent.front().at > kReserve)
            line.recent.pop_front();
        if (line.recent.empty()) return 0;
        // Once the row shows up the player is in the active count; forget the
        // reservation so a quick leave frees the slot at once.
        const auto rows = InstanceRegistry::GetAllActiveInstancePlayers();
        line.recent.erase(std::remove_if(line.recent.begin(), line.recent.end(), [&](const Admit& a) {
            return std::any_of(rows.begin(), rows.end(), [&](const auto& r) {
                return r.instance_id == instance_id && r.session_guid == a.guid;
            });
        }), line.recent.end());
        return (int)line.recent.size();
    }

    bool HookHolds(Line& line, const std::string& map_name, const InstanceInfo& target,
                   int players, int cap) {
        if (!ServiceHooks::Enabled("instance_admit")) return false;
        const auto now = Clock::now();
        if (line.hook_at != Clock::time_point{} && now - line.hook_at < kHookReuse)
            return line.hook_hold;
        nlohmann::json req = {
            {"map", map_name}, {"instance_id", target.instance_id},
            {"players", players}, {"max_players", cap},
            {"waiting", (int)line.waiting.size()},
        };
        auto d = ServiceHooks::Call("instance_admit", req);
        line.hook_at   = now;
        line.hook_hold = d && d->is_object() && d->contains("admit")
                         && (*d)["admit"].is_boolean() && !(*d)["admit"].get<bool>();
        return line.hook_hold;
    }
}

InstanceAdmission::Result InstanceAdmission::Poll(const std::string& map_name,
                                                  const InstanceInfo& target,
                                                  const std::string& session_guid,
                                                  int fallback_cap,
                                                  int* position_out, int* line_length_out) {
    std::lock_guard<std::mutex> lock(g_mutex);
    Line& line = g_lines[map_name];

    auto it = std::find(line.waiting.begin(), line.waiting.end(), session_guid);
    if (it == line.waiting.end()) {
        line.waiting.push_back(session_guid);
        it = std::prev(line.waiting.end());
    }
    const int pos = (int)(it - line.waiting.begin());   // 0-based

    const int cap     = target.max_players > 0 ? target.max_players : fallback_cap;
    const int players = InstanceRegistry::GetActivePlayerCount(target.instance_id);
    int free_slots    = cap > 0 ? cap - players - PendingAdmits(line, target.instance_id) : 1 << 20;
    if (free_slots > 0 && HookHolds(line, map_name, target, players, cap)) free_slots = 0;

    if (pos < free_slots) {
        line.waiting.erase(it);
        line.recent.push_back({session_guid, Clock::now()});
        if (pos > 0 || !line.waiting.empty())
            Logger::Log("travel", "[InstanceAdmission] %s admitted to '%s' (%d/%d, %zu still waiting)\n",
                session_guid.c_str(), map_name.c_str(), players, cap, line.waiting.size());
        return Result::Admit;
    }
    if (position_out)    *position_out    = pos + 1;
    if (line_length_out) *line_length_out = (int)line.waiting.size();
    return Result::Wait;
}

bool InstanceAdmission::Leave(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(g_mutex);
    bool found = false;
    for (auto& [map, line] : g_lines) {
        auto it = std::find(line.waiting.begin(), line.waiting.end(), session_guid);
        if (it != line.waiting.end()) { line.waiting.erase(it); found = true; }
    }
    return found;
}

bool InstanceAdmission::IsWaiting(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& [map, line] : g_lines)
        if (std::find(line.waiting.begin(), line.waiting.end(), session_guid) != line.waiting.end())
            return true;
    return false;
}
