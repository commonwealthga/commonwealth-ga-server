#include "src/ControlServer/MatchmakingService/Rules/DoubleAgentRule.hpp"
#include "src/ControlServer/Logger.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <random>
#include <sstream>
#include <utility>

namespace {

// Shape table — index = total - 2. (tf1_count, tf2_count).
// TF1 = attackers (PvE coop side), TF2 = defenders (the agents).
constexpr std::array<std::pair<uint8_t, uint8_t>, 9> kShapeTable = {{
    {1, 1},  // total 2  — 1v1
    {2, 1},  // total 3  — 2v1
    {2, 2},  // total 4  — 2v2
    {3, 2},  // total 5  — 3v2
    {4, 2},  // total 6  — 4v2
    {4, 3},  // total 7  — 4v3
    {5, 3},  // total 8  — 5v3
    {5, 4},  // total 9  — 5v4
    {6, 4},  // total 10 — 6v4
}};

constexpr uint32_t kMinTotal = 2;
constexpr uint32_t kMaxTotal = 10;

}  // namespace

std::optional<MatchResult> DoubleAgentRule::Evaluate(
    const std::vector<QueuedPlayer>& players,
    const std::vector<RunningInstance>& /*instances*/)
{
    // Late-join lockout: ignore READY instances entirely. Every pop
    // spawns fresh. (`instances` is intentionally unused.)

    if (players.size() < kMinTotal) {
        // Below shape-table floor — queue waits for more bodies.
        return std::nullopt;
    }

    const uint32_t total = (uint32_t)std::min<size_t>(players.size(), kMaxTotal);
    const auto [tf1_n, tf2_n] = kShapeTable[total - kMinTotal];

    // Shuffle a working copy in full so the random selection draws
    // uniformly from the whole queue — not just the first `total`
    // arrivals (which would bias against later joiners on surplus pops).
    std::vector<QueuedPlayer> shuffled = players;
    std::mt19937 rng(
        (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count());
    std::shuffle(shuffled.begin(), shuffled.end(), rng);

    MatchResult r;
    std::ostringstream tf1_log;
    for (uint32_t i = 0; i < tf1_n; ++i) {
        const auto& p = shuffled[i];
        r.session_guids.push_back(p.session_guid);
        r.task_force_assignments[p.session_guid] = 1;
        r.profile_ids[p.session_guid]            = p.profile_id;
        if (i) tf1_log << ",";
        tf1_log << p.session_guid;
    }
    std::ostringstream tf2_log;
    for (uint32_t i = tf1_n; i < tf1_n + tf2_n; ++i) {
        const auto& p = shuffled[i];
        r.session_guids.push_back(p.session_guid);
        r.task_force_assignments[p.session_guid] = 2;
        r.profile_ids[p.session_guid]            = p.profile_id;
        if (i > tf1_n) tf2_log << ",";
        tf2_log << p.session_guid;
    }

    r.cap_override = total;
    // map_name / game_mode left empty: MatchmakingService fills via
    // PickRandomMapPoolEntryForCount(queue, session_guids.size()).

    Logger::Log("queue-pop",
        "[Matchmaking] DA roster queue=%u total=%u tf1=%u tf2=%u TF1=[%s] TF2=[%s] cap_override=%u\n",
        cfg_.queue_id, total, (unsigned)tf1_n, (unsigned)tf2_n,
        tf1_log.str().c_str(), tf2_log.str().c_str(), total);

    return r;
}
