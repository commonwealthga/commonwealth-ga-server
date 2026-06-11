#include "src/ControlServer/MatchmakingService/Rules/VersusSidesRule.hpp"
#include "src/ControlServer/MatchmakingService/RuleSupport.hpp"

using mm::PartiesByWaitAsc;

std::optional<MatchResult> VersusSidesRule::Evaluate(
    const std::vector<QueuedParty>& parties,
    const std::vector<RunningInstance>& /*instances*/) {
    // A side IS a party; never join an existing instance. `instances` unused.

    if (parties.size() < 2) return std::nullopt;  // need two sides

    auto ordered = PartiesByWaitAsc(parties);
    const QueuedParty* a = ordered[0];  // longest-waiting -> TF1
    const QueuedParty* b = ordered[1];  // next           -> TF2

    MatchResult r;
    r.access_mode     = AccessMode::PartyLocked;
    r.owner_party_ids = {a->party_id, b->party_id};

    auto place = [&](const QueuedParty* p, int tf) {
        r.consumed_party_ids.push_back(p->party_id);
        for (const auto& m : p->members) {
            r.session_guids.push_back(m.session_guid);
            r.task_force_assignments[m.session_guid] = tf;
            r.profile_ids[m.session_guid] = m.profile_id;
        }
    };
    place(a, 1);
    place(b, 2);
    // map/mode left empty -> orchestrator fills from pool by session count.
    return r;
}
