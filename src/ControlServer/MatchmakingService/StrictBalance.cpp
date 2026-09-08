#include "src/ControlServer/MatchmakingService/StrictBalance.hpp"

#include "src/ControlServer/MatchmakingService/MmrSwap.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace StrictBalance {

namespace {

// A party's claim to be spared is its strongest member's, on both keys.
uint32_t PartyExclusions(const QueuedParty* p) {
    uint32_t x = 0;
    for (const auto& m : p->members) x = std::max(x, m.fairness.exclusion_count);
    return x;
}

double PartyExclusionRate(const QueuedParty* p) {
    double x = 0.0;
    for (const auto& m : p->members) x = std::max(x, m.fairness.exclusion_rate());
    return x;
}

// Absolute balance-cost ceiling for admitting a late-join pair. PlanPairJoin
// only runs under pair_backfill, which is merc-only, so a constant is already
// scoped. A same-class pair moves its own class gap and the overall gap
// together, so cost ~= 3x the mean-MMR gap (2x class + 1x overall): 300 here
// is the tuned 100-point mean-gap allowance expressed in cost.
constexpr double kLateJoinCostSlack = 300.0;

double MeanDiff(double s1, int n1, double s2, int n2) {
    if (n1 <= 0 || n2 <= 0) return 0.0;
    return std::fabs(s1 / n1 - s2 / n2);
}

int ClassHeads(const TeamSeed& t, uint32_t cls) {
    auto it = t.class_counts.find(cls);
    return it == t.class_counts.end() ? 0 : it->second;
}

double ClassMmr(const TeamSeed& t, uint32_t cls) {
    auto it = t.class_mmr_sum.find(cls);
    return it == t.class_mmr_sum.end() ? 0.0 : it->second;
}

// Mirrors MmrSwap::BalanceCost for two side aggregates: headcount-weighted
// per-class mean-MMR gap (weighted up) plus the overall mean-MMR gap. Equal
// totals with one side holding every good medic score badly here.
double BalanceCost(const TeamSeed& a, const TeamSeed& b) {
    const int heads_total = a.size + b.size;
    double class_gap = 0.0;
    if (heads_total > 0) {
        std::vector<uint32_t> classes;
        for (const auto& [cls, n] : a.class_counts) classes.push_back(cls);
        for (const auto& [cls, n] : b.class_counts)
            if (!a.class_counts.count(cls)) classes.push_back(cls);
        for (uint32_t cls : classes) {
            const int n1 = ClassHeads(a, cls), n2 = ClassHeads(b, cls);
            class_gap += (double)(n1 + n2)
                       * MeanDiff(ClassMmr(a, cls), n1, ClassMmr(b, cls), n2);
        }
        class_gap /= (double)heads_total;
    }
    return MmrSwap::kClassBalanceWeight * class_gap
         + MeanDiff(a.mmr_sum, a.size, b.mmr_sum, b.size);
}

// Copy of `t` with one member of `cls` at `mmr` added.
TeamSeed WithMember(const TeamSeed& t, uint32_t cls, double mmr) {
    TeamSeed out = t;
    out.size += 1;
    out.mmr_sum += mmr;
    out.class_counts[cls] += 1;
    out.class_mmr_sum[cls] += mmr;
    return out;
}

}  // namespace

std::vector<const QueuedParty*> PartiesByPriority(
    const std::vector<QueuedParty>& parties) {
    std::vector<const QueuedParty*> out;
    out.reserve(parties.size());
    for (const auto& p : parties) out.push_back(&p);
    std::stable_sort(out.begin(), out.end(),
        [](const QueuedParty* a, const QueuedParty* b) {
            const uint32_t ea = PartyExclusions(a), eb = PartyExclusions(b);
            if (ea != eb) return ea > eb;
            // Spread the cost proportionally: whoever has been excluded the
            // smallest share of the time is the one who can afford to pay.
            const double ra = PartyExclusionRate(a), rb = PartyExclusionRate(b);
            if (std::fabs(ra - rb) > 1e-9) return ra > rb;
            return a->joined_at < b->joined_at;
        });
    return out;
}

std::vector<const QueuedParty*> SelectClassEqualSubset(
    const std::vector<QueuedParty>& parties, int cap) {
    auto ordered = PartiesByPriority(parties);

    // Greedy add whole parties under the cap (priority order preserved).
    std::vector<const QueuedParty*> chosen;
    int used = 0;
    for (const QueuedParty* p : ordered) {
        const int need = (int)p->size();
        if (cap > 0 && used + need > cap) continue;
        chosen.push_back(p);
        used += need;
    }

    // Repair: drop players until every class count is even. Deterministic:
    // lowest class id first, lowest-priority victim (chosen is in priority
    // order, so scan from the back).
    for (;;) {
        std::unordered_map<uint32_t, int> counts;
        for (const QueuedParty* p : chosen)
            for (const auto& m : p->members) counts[m.profile_id] += 1;
        uint32_t odd_class = 0;
        bool any_odd = false;
        for (const auto& [cls, n] : counts) {
            if (n % 2 == 0) continue;
            if (!any_odd || cls < odd_class) { odd_class = cls; any_odd = true; }
        }
        if (!any_odd) break;

        auto victim = chosen.end();
        for (auto it = chosen.rbegin(); it != chosen.rend(); ++it) {
            if ((*it)->size() == 1 && (*it)->members.front().profile_id == odd_class) {
                victim = std::next(it).base();
                break;
            }
        }
        if (victim == chosen.end()) {
            for (auto it = chosen.rbegin(); it != chosen.rend(); ++it) {
                bool has = false;
                for (const auto& m : (*it)->members)
                    if (m.profile_id == odd_class) { has = true; break; }
                if (has) { victim = std::next(it).base(); break; }
            }
        }
        if (victim == chosen.end()) break;  // defensive
        chosen.erase(victim);
    }
    return chosen;
}

std::vector<Invite> PlanDeficitBackfill(
    const std::vector<QueuedParty>& parties, const RunningInstance& inst) {
    std::vector<Invite> out;
    auto ordered = PartiesByPriority(parties);

    // Union of class ids present on either side, ascending for determinism.
    std::vector<uint32_t> classes;
    for (const auto& [cls, n] : inst.team1.class_counts) classes.push_back(cls);
    for (const auto& [cls, n] : inst.team2.class_counts)
        if (!inst.team1.class_counts.count(cls)) classes.push_back(cls);
    std::sort(classes.begin(), classes.end());

    int seats = inst.seats_free();  // -1 = unlimited
    std::unordered_set<const QueuedParty*> taken;
    for (uint32_t cls : classes) {
        const int c1 = inst.team1.class_counts.count(cls) ? inst.team1.class_counts.at(cls) : 0;
        const int c2 = inst.team2.class_counts.count(cls) ? inst.team2.class_counts.at(cls) : 0;
        int deficit = std::abs(c1 - c2);
        const int tf = (c1 < c2) ? 1 : 2;
        for (const QueuedParty* p : ordered) {
            if (deficit == 0 || seats == 0) break;
            if (p->size() != 1 || taken.count(p)) continue;
            if (p->members.front().profile_id != cls) continue;
            out.push_back({p, tf});
            taken.insert(p);
            deficit -= 1;
            if (seats > 0) seats -= 1;
        }
    }
    return out;
}

std::vector<Invite> PlanPairJoin(
    const std::vector<QueuedParty>& parties, const RunningInstance& inst) {
    if (inst.team1.size != inst.team2.size) return {};
    if (inst.team1.class_counts.size() != inst.team2.class_counts.size()) return {};
    for (const auto& [cls, n] : inst.team1.class_counts) {
        auto it = inst.team2.class_counts.find(cls);
        if (it == inst.team2.class_counts.end() || it->second != n) return {};
    }
    const int seats = inst.seats_free();
    if (seats >= 0 && seats < 2) return {};

    const double cur = BalanceCost(inst.team1, inst.team2);

    auto ordered = PartiesByPriority(parties);
    std::vector<const QueuedParty*> solos;
    for (const QueuedParty* p : ordered)
        if (p->size() == 1) solos.push_back(p);

    // First same-class pair (in priority order) whose better orientation does
    // not widen the balance cost wins. Because the pair shares a class, the
    // orientation is decided by that class's own strength gap, not just totals.
    for (size_t i = 0; i < solos.size(); ++i) {
        for (size_t j = i + 1; j < solos.size(); ++j) {
            const auto& a = solos[i]->members.front();
            const auto& b = solos[j]->members.front();
            if (a.profile_id != b.profile_id) continue;
            const double d1 = BalanceCost(WithMember(inst.team1, a.profile_id, a.mmr),
                                          WithMember(inst.team2, b.profile_id, b.mmr));
            const double d2 = BalanceCost(WithMember(inst.team1, b.profile_id, b.mmr),
                                          WithMember(inst.team2, a.profile_id, a.mmr));
            // Admit if it doesn't widen the gap, or leaves it under the slack.
            if (std::min(d1, d2) > std::max(cur, kLateJoinCostSlack) + 1e-9) continue;
            const int a_tf = (d1 <= d2) ? 1 : 2;
            return {{solos[i], a_tf}, {solos[j], a_tf == 1 ? 2 : 1}};
        }
    }
    return {};
}

}  // namespace StrictBalance
