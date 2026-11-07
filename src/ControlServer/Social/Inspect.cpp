#include "src/ControlServer/Social/Inspect.hpp"
#include <chrono>
#include <mutex>
#include <unordered_map>

namespace Inspect {

// valid-value group 59 ids
static constexpr uint32_t kMapTypePublic = 324;

uint32_t MapTypeFor(const std::string& game_mode) {
    // Dome City (and any other TgGame_City map) = public city.
    if (game_mode == "TgGame.TgGame_City") return kMapTypePublic;
    return 0;
}

// How long after the last inspect request the keepalive push shows the target.
// The client sends no "profile closed" packet; it re-sends 0x01C5 on every inspect open.
static constexpr auto kInspectWindow = std::chrono::seconds(45);

namespace {
struct Target { int64_t char_id = 0; std::chrono::steady_clock::time_point until{}; };
std::mutex g_mu;
std::unordered_map<std::string, Target> g_targets;
}

int64_t NoteRequest(const std::string& session_guid, int64_t viewer_char, int64_t target_char) {
    if (session_guid.empty() || target_char <= 0 || viewer_char <= 0) return 0;
    std::lock_guard<std::mutex> lk(g_mu);
    if (target_char == viewer_char) { g_targets.erase(session_guid); return viewer_char; }  // own profile opened
    if (g_targets.size() > 4096) g_targets.clear();
    g_targets[session_guid] = {target_char, std::chrono::steady_clock::now() + kInspectWindow};
    return target_char;
}

int64_t ActiveTarget(const std::string& session_guid) {
    std::lock_guard<std::mutex> lk(g_mu);
    auto it = g_targets.find(session_guid);
    if (it == g_targets.end()) return 0;
    if (std::chrono::steady_clock::now() >= it->second.until) { g_targets.erase(it); return 0; }
    return it->second.char_id;
}

}  // namespace Inspect
