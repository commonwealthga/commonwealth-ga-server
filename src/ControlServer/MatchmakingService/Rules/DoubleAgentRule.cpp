#include "src/ControlServer/MatchmakingService/Rules/DoubleAgentRule.hpp"
#include "src/ControlServer/MatchmakingService/RuleSupport.hpp"

#include <algorithm>
#include <array>
#include <utility>

using mm::PartiesByWaitAsc;

namespace {

// Index = total - 2. (tf1_count, tf2_count). TF1 = attackers, TF2 = defenders.
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

// Fill TF1 (cap tf1_n) and TF2 (cap tf2_n) from `chosen` parties, keeping each
// party on a single side when its members fit, spilling overflow otherwise.
// Returns guid -> tf. Capacities are exactly consumed (sum == total).
std::unordered_map<std::string, int> AssignShape(
    const std::vector<const QueuedParty*>& chosen, int tf1_n, int tf2_n) {

    // Largest parties first so they claim a whole side before fragments fill in.
    std::vector<const QueuedParty*> order = chosen;
    std::stable_sort(order.begin(), order.end(),
        [](const QueuedParty* a, const QueuedParty* b) { return a->size() > b->size(); });

    int rem1 = tf1_n, rem2 = tf2_n;
    std::unordered_map<std::string, int> out;
    for (const QueuedParty* p : order) {
        const int n = (int)p->size();
        const bool fit1 = rem1 >= n, fit2 = rem2 >= n;
        int primary;
        if (fit1 && fit2)      primary = (rem1 >= rem2) ? 1 : 2;
        else if (fit1)         primary = 1;
        else if (fit2)         primary = 2;
        else                   primary = (rem1 >= rem2) ? 1 : 2;  // must spill
        const int secondary = (primary == 1) ? 2 : 1;

        for (const auto& m : p->members) {
            int side;
            if (primary == 1 ? rem1 > 0 : rem2 > 0)        side = primary;
            else if (secondary == 1 ? rem1 > 0 : rem2 > 0) side = secondary;
            else                                           side = primary;  // unreachable
            out[m.session_guid] = side;
            if (side == 1) rem1--; else rem2--;
        }
    }
    return out;
}

}  // namespace

std::optional<MatchResult> DoubleAgentRule::Evaluate(
    const std::vector<QueuedParty>& parties,
    const std::vector<RunningInstance>& /*instances*/) {
    // Late-join lockout: never join existing instances. `instances` unused.

    if (parties.empty()) return std::nullopt;

    // Select whole parties (longest-waiting first) up to kMaxTotal seats.
    auto ordered = PartiesByWaitAsc(parties);
    std::vector<const QueuedParty*> chosen;
    int total = 0;
    for (const QueuedParty* p : ordered) {
        const int n = (int)p->size();
        if (n > (int)kMaxTotal) continue;          // can't ever fit; leave queued
        if (total + n > (int)kMaxTotal) continue;  // try a smaller party instead
        chosen.push_back(p);
        total += n;
    }

    if ((uint32_t)total < kMinTotal) return std::nullopt;  // wait for more bodies

    const auto [tf1_n, tf2_n] = kShapeTable[total - kMinTotal];
    auto assignment = AssignShape(chosen, tf1_n, tf2_n);

    MatchResult r;
    r.access_mode  = AccessMode::Sealed;
    r.cap_override = (uint32_t)total;  // seal at popped size
    for (const QueuedParty* p : chosen) {
        r.consumed_party_ids.push_back(p->party_id);
        for (const auto& m : p->members) {
            const int tf = assignment.count(m.session_guid) ? assignment[m.session_guid] : 1;
            r.session_guids.push_back(m.session_guid);
            r.task_force_assignments[m.session_guid] = tf;
            r.profile_ids[m.session_guid] = m.profile_id;
        }
    }
    // map/mode left empty -> orchestrator fills from pool by session count.
    return r;
}
