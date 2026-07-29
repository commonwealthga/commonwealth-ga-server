#include "src/ControlServer/MatchmakingService/Rules/CoopMatchRule.hpp"
#include "src/ControlServer/MatchmakingService/RuleSupport.hpp"

#include <algorithm>

using mm::PartiesByWaitAsc;
using mm::PackParties;
using mm::BuildResult;

namespace {

// Split a party list into (teams, solos).
void Partition(const std::vector<QueuedParty>& parties,
               std::vector<QueuedParty>& teams,
               std::vector<QueuedParty>& solos) {
    for (const auto& p : parties) {
        if (p.is_team) teams.push_back(p);
        else           solos.push_back(p);
    }
}

}  // namespace

MatchResult CoopMatchRule::BuildOwnMatch(const QueuedParty& team) const {
    // One party (team, or solo-locked player), its own fresh PARTY_LOCKED
    // instance. Cap stays the queue cap (left to the orchestrator) so
    // in-mission invites can still grow it.
    std::vector<QueuedParty> one{team};
    auto ordered = PartiesByWaitAsc(one);
    MatchResult r = BuildResult(
        ordered, cfg_.taskforce_policy, cfg_.team_side_policy,
        /*seed1=*/{}, /*seed2=*/{},
        AccessMode::PartyLocked, /*owners=*/{team.party_id});
    // map/mode left empty -> filled from pool by the orchestrator.
    return r;
}

std::optional<MatchResult> CoopMatchRule::PlacePool(
    const std::vector<QueuedParty>& parties,
    const std::vector<RunningInstance>& instances) const {

    if (parties.empty()) return std::nullopt;
    auto ordered = PartiesByWaitAsc(parties);

    // 1. Drop-in: first OPEN instance with room for at least one whole party.
    for (const auto& inst : instances) {
        if (inst.access_mode != AccessMode::Open) continue;  // never enter locked/sealed
        const int seats = inst.seats_free();                 // -1 = unlimited
        auto chosen = PackParties(ordered, seats);
        if (chosen.empty()) continue;

        MatchResult r = BuildResult(
            chosen, cfg_.taskforce_policy, cfg_.team_side_policy,
            inst.team1, inst.team2, AccessMode::Open, /*owners=*/{});
        r.existing_instance_id = inst.instance_id;
        r.map_name  = inst.map_name;
        r.game_mode = inst.game_mode;
        return r;
    }

    // 2. Fresh OPEN spawn. Cap by the queue's per-instance ceiling.
    const uint32_t cap = mm::QueueInstanceCap(cfg_);
    auto chosen = PackParties(ordered, cap > 0 ? (int)cap : -1);
    if (chosen.empty()) return std::nullopt;

    return BuildResult(
        chosen, cfg_.taskforce_policy, cfg_.team_side_policy,
        /*seed1=*/{}, /*seed2=*/{}, AccessMode::Open, /*owners=*/{});
}

std::optional<MatchResult> CoopMatchRule::Evaluate(
    const std::vector<QueuedParty>& parties,
    const std::vector<RunningInstance>& instances) {

    if (parties.empty()) return std::nullopt;

    std::vector<QueuedParty> teams, solos;
    Partition(parties, teams, solos);

    // -togglesolomode: a solo party whose player wants a private match pops
    // its own fresh PARTY_LOCKED instance (like a team under OwnMatch) and
    // never enters the drop-in pool. Only where a 1-player spawn is possible
    // (min_players_to_pop <= 1) — in group queues (merc/ddr/sr) the flag is
    // inert and the player is matched normally. TryPop recurses after each
    // pop, so remaining locked solos/teams drain in the same event.
    if (cfg_.min_players_to_pop <= 1) {
        std::vector<QueuedParty> locked;
        solos.erase(std::remove_if(solos.begin(), solos.end(),
            [&](const QueuedParty& p) {
                const bool wants = !p.members.empty() && p.members.front().solo_lock;
                if (wants) locked.push_back(p);
                return wants;
            }), solos.end());
        if (!locked.empty()) {
            auto ordered = PartiesByWaitAsc(locked);
            return BuildOwnMatch(*ordered.front());
        }
    }

    switch (cfg_.team_policy) {
        case TeamPolicy::OwnMatch: {
            // A team always gets its own fresh PARTY_LOCKED match. Pop the
            // longest-waiting team first; solos handled on a later Evaluate.
            if (!teams.empty()) {
                auto ordered = PartiesByWaitAsc(teams);
                return BuildOwnMatch(*ordered.front());
            }
            // Solos only — drop into OPEN instances or fresh-spawn.
            return PlacePool(solos, instances);
        }
        case TeamPolicy::Block:
            // Teams rejected upstream; defensively ignore any that slipped in.
            return PlacePool(solos, instances);
        case TeamPolicy::Mixed: {
            // teams + solos, NOT `parties` — solo-locked parties were stripped
            // from `solos` above and must stay out of the shared pool.
            // PlacePool re-orders by wait time, so concatenation order is fine.
            std::vector<QueuedParty> pool = teams;
            pool.insert(pool.end(), solos.begin(), solos.end());
            return PlacePool(pool, instances);
        }
        case TeamPolicy::VersusSides:
            // Misconfiguration — VersusSides queues should use VersusSidesRule.
            return std::nullopt;
    }
    return std::nullopt;
}
