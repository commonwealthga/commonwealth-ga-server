#include "src/ControlServer/MatchmakingService/Rules/DoubleAgentRule.hpp"
#include "src/ControlServer/MatchmakingService/RuleSupport.hpp"
#include "src/ControlServer/MatchmakingService/DefenderRotation.hpp"

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
    // TF2 (defenders) is the wanted side — hand it to the strongest rotation
    // claim, never splitting a party unless no subset fills the seats.
    bool spilled = false;
    auto assignment = DefenderRotation::Assign(chosen, tf1_n, tf2_n, &spilled);

    MatchResult r;
    r.access_mode  = AccessMode::Sealed;
    r.cap_override = (uint32_t)total;  // seal at popped size
    r.cohesion_spilled = spilled;
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
