#include "src/ControlServer/MatchmakingService/SidePlacement.hpp"

#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/MatchmakingService/MmrSwap.hpp"

#include <algorithm>
#include <cmath>

namespace SidePlacement {

float HealValue(uint32_t profile_id, int task_force) {
    if (profile_id == kProfileMedic) return 1.0f;
    if (profile_id == kProfileRobotics) return task_force == 2 ? 0.85f : 0.75f;
    return 0.0f;
}

int ClassImbalance(const TeamSeed& a, const TeamSeed& b) {
    int diff = 0;
    for (const auto& [cls, n] : a.class_counts) {
        auto it = b.class_counts.find(cls);
        diff += std::abs(n - (it != b.class_counts.end() ? it->second : 0));
    }
    for (const auto& [cls, n] : b.class_counts) {
        if (a.class_counts.find(cls) == a.class_counts.end()) diff += std::abs(n);
    }
    return diff;
}

float Imbalance(const TeamSeed& a, const TeamSeed& b) {
    const float heal_diff = std::abs(a.heal_score - b.heal_score);
    const float size_diff = (float)std::abs(a.size - b.size);
    return kClassWeight * (float)ClassImbalance(a, b) + kHealWeight * heal_diff + kSizeWeight * size_diff;
}

namespace {

void AddSlot(TeamSeed& seed, const Slot& s, int tf) {
    seed.size += 1;
    seed.heal_score += HealValue(s.profile_id, tf);
    seed.class_counts[s.profile_id] += 1;
    seed.mmr_sum += s.mmr;
}

// Place one slot on whichever side yields lower post-placement imbalance.
// Tie -> lower-MMR side, then TF1. Updates the chosen seed, records the
// assignment.
int PlaceSlotGreedy(const Slot& s, TeamSeed& seed1, TeamSeed& seed2) {
    TeamSeed try1 = seed1; AddSlot(try1, s, 1);
    TeamSeed try2 = seed2; AddSlot(try2, s, 2);
    const float cost1 = Imbalance(try1, seed2);
    const float cost2 = Imbalance(seed1, try2);
    if (cost2 < cost1) { seed2 = try2; return 2; }
    if (cost1 == cost2 && seed2.mmr_sum < seed1.mmr_sum) { seed2 = try2; return 2; }
    seed1 = try1; return 1;
}

// Order groups largest-first (stable on ties).
std::vector<size_t> GroupsBySizeDesc(const std::vector<Group>& groups) {
    std::vector<size_t> idx(groups.size());
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
        return groups[a].members.size() > groups[b].members.size();
    });
    return idx;
}

// Place every group whole (atomic): each goes to the side minimising imbalance.
// Largest-first. Fills `out`, leaves final seeds in seed1/seed2.
void AssignWhole(const std::vector<Group>& groups,
                 TeamSeed& seed1, TeamSeed& seed2,
                 std::unordered_map<std::string, int>& out) {
    for (size_t gi : GroupsBySizeDesc(groups)) {
        const Group& g = groups[gi];
        if (g.members.empty()) continue;
        TeamSeed s1a = seed1, s2a = seed2;
        for (const auto& m : g.members) AddSlot(s1a, m, 1);
        const float cost1 = Imbalance(s1a, seed2);
        TeamSeed s1b = seed1, s2b = seed2;
        for (const auto& m : g.members) AddSlot(s2b, m, 2);
        const float cost2 = Imbalance(seed1, s2b);
        // Tie -> the MMR-weaker side (was: always TF1).
        const int side = (cost2 < cost1) ? 2
            : (cost1 == cost2 && seed2.mmr_sum < seed1.mmr_sum) ? 2 : 1;
        if (side == 1) seed1 = s1a; else seed2 = s2b;
        for (const auto& m : g.members) out[m.guid] = side;
    }
}

// Place every slot individually (heal tier desc), ignoring party grouping.
void AssignIndividual(const std::vector<Group>& groups,
                      TeamSeed& seed1, TeamSeed& seed2,
                      std::unordered_map<std::string, int>& out) {
    std::vector<const Slot*> slots;
    for (const auto& g : groups)
        for (const auto& m : g.members) slots.push_back(&m);
    std::stable_sort(slots.begin(), slots.end(), [](const Slot* a, const Slot* b) {
        return std::max(HealValue(a->profile_id, 1), HealValue(a->profile_id, 2))
             > std::max(HealValue(b->profile_id, 1), HealValue(b->profile_id, 2));
    });
    for (const Slot* s : slots) out[s->guid] = PlaceSlotGreedy(*s, seed1, seed2);
}

// BalancedPvp only: MMR post-pass over solo players. Same-class swaps keep
// the class/heal/size result of the placement above fully intact; party
// members are never moved. seed_diff = live-roster MMR imbalance, so the
// newcomers compensate for the match they are joining.
std::unordered_map<std::string, int> Finish(
    TaskforcePolicy tf_policy,
    const std::vector<Group>& groups,
    std::unordered_map<std::string, int> out,
    double seed_diff) {
    if (tf_policy == TaskforcePolicy::BalancedPvp) {
        std::vector<MmrSwap::Player> mp;
        for (const auto& g : groups)
            for (const auto& m : g.members)
                mp.push_back({m.guid, m.profile_id, m.mmr,
                              m.solo && g.members.size() == 1});
        MmrSwap::BalanceByMmr(mp, out, seed_diff);
    }
    for (const auto& g : groups)
        for (const auto& m : g.members)
            Logger::Log("team-balance",
                "[SidePlacement]   guid=%s profile=%u mmr=%.0f -> tf=%d\n",
                m.guid.c_str(), m.profile_id, m.mmr, out[m.guid]);
    return out;
}

}  // namespace

std::unordered_map<std::string, int> Assign(
    TaskforcePolicy tf_policy,
    TeamSidePolicy  side_policy,
    const std::vector<Group>& groups,
    TeamSeed seed1,
    TeamSeed seed2) {

    std::unordered_map<std::string, int> out;

    // Live-roster MMR imbalance, captured before placement mutates the seeds.
    const double seed_diff = seed1.mmr_sum - seed2.mmr_sum;

    Logger::Log("team-balance",
        "[SidePlacement] Assign groups=%zu seed1=(%.2f,%d,%.0f) seed2=(%.2f,%d,%.0f)\n",
        groups.size(), seed1.heal_score, seed1.size, seed1.mmr_sum,
        seed2.heal_score, seed2.size, seed2.mmr_sum);

    // Pinned: one side, side_policy irrelevant.
    if (tf_policy == TaskforcePolicy::Pinned1 || tf_policy == TaskforcePolicy::Pinned2) {
        const int tf = (tf_policy == TaskforcePolicy::Pinned1) ? 1 : 2;
        for (const auto& g : groups)
            for (const auto& m : g.members) out[m.guid] = tf;
        return out;
    }

    if (side_policy == TeamSidePolicy::Ignore) {
        AssignIndividual(groups, seed1, seed2, out);
        return Finish(tf_policy, groups, std::move(out), seed_diff);
    }

    if (side_policy == TeamSidePolicy::Required) {
        AssignWhole(groups, seed1, seed2, out);
        return Finish(tf_policy, groups, std::move(out), seed_diff);
    }

    // Preferred: keep parties whole unless doing so leaves a class imbalance
    // that individual placement would fix. Placement itself is size-first;
    // this gate stays class-only, so a size-only imbalance never breaks a
    // party apart.
    TeamSeed ws1 = seed1, ws2 = seed2;
    std::unordered_map<std::string, int> whole;
    AssignWhole(groups, ws1, ws2, whole);

    TeamSeed is1 = seed1, is2 = seed2;
    std::unordered_map<std::string, int> indiv;
    AssignIndividual(groups, is1, is2, indiv);

    if (ClassImbalance(is1, is2) < ClassImbalance(ws1, ws2))
        return Finish(tf_policy, groups, std::move(indiv), seed_diff);
    return Finish(tf_policy, groups, std::move(whole), seed_diff);
}

}  // namespace SidePlacement
