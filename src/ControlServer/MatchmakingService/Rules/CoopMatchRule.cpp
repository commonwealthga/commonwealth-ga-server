#include "src/ControlServer/MatchmakingService/Rules/CoopMatchRule.hpp"

#include <unordered_map>
#include "src/ControlServer/MatchmakingService/RuleSupport.hpp"
#include "src/ControlServer/MatchmakingService/StrictBalance.hpp"
#include "src/ControlServer/Logger.hpp"

#include <algorithm>
#include <ctime>
#include "src/ControlServer/MatchModeFilter/MatchModeFilter.hpp"

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

// Distinct profile_ids among the given parties' members.
uint32_t DistinctClasses(const std::vector<const QueuedParty*>& parties) {
    std::vector<uint32_t> seen;
    for (const auto* p : parties)
        for (const auto& m : p->members)
            if (std::find(seen.begin(), seen.end(), m.profile_id) == seen.end())
                seen.push_back(m.profile_id);
    return (uint32_t)seen.size();
}

// PackParties with a per-class limit for the whole match (max_class_per_side
// x 2, starting from the instance's current class counts). limit 0 = plain
// PackParties.
std::vector<const QueuedParty*> PackPartiesClassCap(
    const std::vector<const QueuedParty*>& ordered, int seat_budget,
    uint32_t per_side, const TeamSeed& seed1, const TeamSeed& seed2) {
    if (per_side == 0) return PackParties(ordered, seat_budget);
    const int limit = 2 * (int)per_side;
    std::unordered_map<uint32_t, int> counts = seed1.class_counts;
    for (const auto& kv : seed2.class_counts) counts[kv.first] += kv.second;
    std::vector<const QueuedParty*> chosen;
    int used = 0;
    for (const QueuedParty* p : ordered) {
        const int need = (int)p->size();
        if (seat_budget >= 0 && used + need > seat_budget) continue;
        std::unordered_map<uint32_t, int> add;
        for (const auto& m : p->members) add[m.profile_id]++;
        bool fits = true;
        for (const auto& kv : add)
            if (counts[kv.first] + kv.second > limit) { fits = false; break; }
        if (!fits) continue;
        for (const auto& kv : add) counts[kv.first] += kv.second;
        chosen.push_back(p);
        used += need;
    }
    return chosen;
}

// A running PvP match older than the client's seal time only takes
// parties whose every member ticked "Volunteer me for late backfills".
bool IsLateMatch(const RunningInstance& inst) {
    return inst.started_at > 0
        && (int64_t)std::time(nullptr) - inst.started_at >= MatchModeFilter::kLateBackfillAfterSecs;
}
bool PartyVolunteers(const QueuedParty& p) {
    for (const auto& m : p.members)
        if (!MatchModeFilter::LateBackfillVolunteer(m.session_guid)) return false;
    return true;
}
std::vector<QueuedParty> VolunteersOnly(const std::vector<QueuedParty>& in) {
    std::vector<QueuedParty> out;
    for (const auto& p : in) if (PartyVolunteers(p)) out.push_back(p);
    return out;
}
std::vector<const QueuedParty*> VolunteersOnly(const std::vector<const QueuedParty*>& in) {
    std::vector<const QueuedParty*> out;
    for (const auto* p : in) if (PartyVolunteers(*p)) out.push_back(p);
    return out;
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

    // skal override difficulty support
    // only if the queue is adept/expert missions aka difficultyid==3000 or 4000
    // otherwise the override is ignored
    if (team.difficulty_override
        && ((cfg_.difficulty_value_id==3000)
         || (cfg_.difficulty_value_id==4000))) {
        r.difficulty_override = team.difficulty_override;
        //Logger::Log ("skal","[CoopMatchRule::BuildOwnMatch] - difficulty override = %.2f/%.2f\n",r.difficulty_override.HP,r.difficulty_override.Dmg);
    }
    return r;
}

std::optional<MatchResult> CoopMatchRule::PlacePool(
    const std::vector<QueuedParty>& parties,
    const std::vector<RunningInstance>& instances) const {

    if (parties.empty()) return std::nullopt;
    auto ordered = PartiesByWaitAsc(parties);

    // Strict knobs are inert unless the queue is BalancedPvp.
    const bool pvp      = cfg_.taskforce_policy == TaskforcePolicy::BalancedPvp;
    const bool strict   = pvp && cfg_.strict_class_balance;
    const bool backfill = pvp && cfg_.late_join_policy == LateJoinPolicy::BackfillOnly;

    // 0. Backfill into BACKFILL_ONLY instances (sealed to drop-in; this is
    //    the only entry path). Class repair first, then the optional
    //    MMR-gated pair admission.
    if (backfill) {
        for (const auto& inst : instances) {
            if (inst.access_mode != AccessMode::BackfillOnly) continue;
            const std::vector<QueuedParty> late_ok = IsLateMatch(inst) ? VolunteersOnly(parties) : parties;
            if (late_ok.empty()) continue;
            auto invites = StrictBalance::PlanDeficitBackfill(late_ok, inst);
            if (invites.empty() && cfg_.pair_backfill)
                invites = StrictBalance::PlanPairJoin(late_ok, inst);
            if (invites.empty()) continue;

            MatchResult r;
            for (const auto& iv : invites) {
                const auto& m = iv.party->members.front();
                r.consumed_party_ids.push_back(iv.party->party_id);
                r.session_guids.push_back(m.session_guid);
                r.task_force_assignments[m.session_guid] = iv.tf;
                r.profile_ids[m.session_guid] = m.profile_id;
                r.mmrs[m.session_guid] = m.mmr;
            }
            r.existing_instance_id = inst.instance_id;
            r.map_name  = inst.map_name;
            r.game_mode = inst.game_mode;
            return r;
        }
    }

    // 1. Drop-in: first OPEN instance with room for at least one whole party.
    for (const auto& inst : instances) {
        if (inst.access_mode != AccessMode::Open) continue;  // never enter locked/sealed
        const int seats = inst.seats_free();                 // -1 = unlimited
        const bool late = pvp && IsLateMatch(inst);          // late backfill = volunteers only
        auto chosen = PackPartiesClassCap(late ? VolunteersOnly(ordered) : ordered, seats,
                                          cfg_.max_class_per_side, inst.team1, inst.team2);
        if (chosen.empty()) continue;

        MatchResult r = BuildResult(
            chosen, cfg_.taskforce_policy, cfg_.team_side_policy,
            inst.team1, inst.team2, AccessMode::Open, /*owners=*/{});
        r.existing_instance_id = inst.instance_id;
        r.map_name  = inst.map_name;
        r.game_mode = inst.game_mode;
        return r;
    }

    // 2. Fresh spawn. Cap by the queue's per-instance ceiling; strict mode
    //    picks the largest all-even-class subset instead of greedy packing.
    const uint32_t cap = mm::QueueInstanceCap(cfg_);
    std::vector<const QueuedParty*> chosen;
    if (strict) {
        chosen = StrictBalance::SelectClassEqualSubset(parties, cap > 0 ? (int)cap : -1);
    } else {
        chosen = PackPartiesClassCap(ordered, cap > 0 ? (int)cap : -1,
                                     cfg_.max_class_per_side, {}, {});
    }
    if (chosen.empty()) return std::nullopt;
    if (cfg_.min_distinct_classes > 0 &&
        DistinctClasses(chosen) < cfg_.min_distinct_classes) return std::nullopt;

    MatchResult r = BuildResult(
        chosen, cfg_.taskforce_policy, cfg_.team_side_policy,
        /*seed1=*/{}, /*seed2=*/{}, AccessMode::Open, /*owners=*/{});
    if (backfill) {
        r.access_mode  = AccessMode::BackfillOnly;
        r.cap_override = (uint32_t)r.session_guids.size();
    }
    return r;
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
