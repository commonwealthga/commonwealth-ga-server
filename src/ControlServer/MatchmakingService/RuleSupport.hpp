#pragma once
//
// RuleSupport — PURE helpers shared by the match rules (and their tests).
// Party packing + MatchResult assembly. No I/O, no singletons.
//
#include "src/ControlServer/MatchmakingService/Domain.hpp"
#include "src/ControlServer/MatchmakingService/SidePlacement.hpp"

#include <algorithm>
#include <vector>

namespace mm {

// Parties ordered longest-waiting first (fair pop order; stable on ties).
inline std::vector<const QueuedParty*>
PartiesByWaitAsc(const std::vector<QueuedParty>& parties) {
    std::vector<const QueuedParty*> out;
    out.reserve(parties.size());
    for (const auto& p : parties) out.push_back(&p);
    std::stable_sort(out.begin(), out.end(),
        [](const QueuedParty* a, const QueuedParty* b) {
            return a->joined_at < b->joined_at;
        });
    return out;
}

// Greedily pack whole parties (longest-waiting first) into a seat budget.
// seat_budget < 0 means unlimited. Parties are atomic: one is taken only if
// ALL its members fit. Returns the chosen parties (original pointers).
inline std::vector<const QueuedParty*>
PackParties(const std::vector<const QueuedParty*>& ordered, int seat_budget) {
    std::vector<const QueuedParty*> chosen;
    int used = 0;
    for (const QueuedParty* p : ordered) {
        const int need = (int)p->size();
        if (seat_budget >= 0 && used + need > seat_budget) continue;  // try next (smaller) party
        chosen.push_back(p);
        used += need;
    }
    return chosen;
}

// Total member count across a party list.
inline int CountMembers(const std::vector<const QueuedParty*>& parties) {
    int n = 0;
    for (const QueuedParty* p : parties) n += (int)p->size();
    return n;
}

// Build a MatchResult for `chosen` parties placed via SidePlacement.
//   tf_policy/side_policy — placement behaviour
//   seed1/seed2           — pre-existing side makeup (instance join) or empty
//   access/owners         — instance access semantics
// map/mode/existing_instance_id are left to the caller to fill.
inline MatchResult BuildResult(
    const std::vector<const QueuedParty*>& chosen,
    TaskforcePolicy tf_policy,
    TeamSidePolicy  side_policy,
    const TeamSeed& seed1,
    const TeamSeed& seed2,
    AccessMode access,
    std::vector<uint64_t> owners) {

    std::vector<SidePlacement::Group> groups;
    groups.reserve(chosen.size());
    for (const QueuedParty* p : chosen) {
        SidePlacement::Group g;
        g.party_id = p->party_id;
        for (const auto& m : p->members)
            g.members.push_back({m.session_guid, m.profile_id, m.mmr,
                                 p->size() == 1});
        groups.push_back(std::move(g));
    }

    auto assignment = SidePlacement::Assign(tf_policy, side_policy, groups, seed1, seed2);

    MatchResult r;
    r.access_mode = access;
    r.owner_party_ids = std::move(owners);
    for (const QueuedParty* p : chosen) {
        r.consumed_party_ids.push_back(p->party_id);
        for (const auto& m : p->members) {
            const int tf = assignment.count(m.session_guid) ? assignment[m.session_guid] : 1;
            r.session_guids.push_back(m.session_guid);
            r.task_force_assignments[m.session_guid] = tf;
            r.profile_ids[m.session_guid] = m.profile_id;
        }
    }
    return r;
}

}  // namespace mm
