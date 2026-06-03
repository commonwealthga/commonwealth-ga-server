#include "src/ControlServer/MatchmakingService/RoleWeightedSplit.hpp"

#include "src/ControlServer/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>

namespace RoleWeightedSplit {

namespace {

std::mt19937& Rng() {
    static std::mt19937 eng{std::random_device{}()};
    return eng;
}

// Cost of placing a player with the given heal-values onto team t given
// the pre-placement (heal1, size1, heal2, size2) state.
float Cost(int t,
           float v_on_1, float v_on_2,
           float heal1, int size1,
           float heal2, int size2)
{
    const float post_h1 = heal1 + (t == 1 ? v_on_1 : 0.0f);
    const float post_h2 = heal2 + (t == 2 ? v_on_2 : 0.0f);
    const int   post_s1 = size1 + (t == 1 ? 1 : 0);
    const int   post_s2 = size2 + (t == 2 ? 1 : 0);

    const float heal_diff_signed = post_h1 - post_h2;
    const int   size_diff_signed = post_s1 - post_s2;
    const float co_aligned =
        std::max(0.0f, heal_diff_signed * static_cast<float>(size_diff_signed));

    return kAlpha * std::fabs(heal_diff_signed)
         + kBeta  * static_cast<float>(std::abs(size_diff_signed))
         + kGamma * co_aligned;
}

int GreedyPlaceOne(uint32_t profile_id,
                   float heal1, int size1,
                   float heal2, int size2)
{
    const float v1 = HealValue(profile_id, 1);
    const float v2 = HealValue(profile_id, 2);

    const float c1 = Cost(1, v1, v2, heal1, size1, heal2, size2);
    const float c2 = Cost(2, v1, v2, heal1, size1, heal2, size2);

    if (c1 < c2) return 1;
    if (c2 < c1) return 2;
    if (size1 < size2) return 1;
    if (size2 < size1) return 2;
    return 1;
}

}  // namespace

float HealValue(uint32_t profile_id, int task_force) {
    switch (profile_id) {
        case kProfileMedic:    return 1.00f;
        case kProfileRobotics: return task_force == 2 ? (0.75f + kRoboticsDefBonus) : 0.75f;
        default:               return 0.00f;
    }
}

int PlaceSingle(uint32_t profile_id, TeamState team1, TeamState team2) {
    const int tf = GreedyPlaceOne(profile_id,
                                  team1.heal_score, team1.size,
                                  team2.heal_score, team2.size);
    Logger::Log("team-balance",
        "[RoleWeightedSplit] PlaceSingle profile=%u team1=(%.2f,%d) team2=(%.2f,%d) -> tf=%d\n",
        profile_id, team1.heal_score, team1.size,
        team2.heal_score, team2.size, tf);
    return tf;
}

std::unordered_map<std::string, int>
ComputeBatchAssignment(const std::vector<PlayerSlot>& players,
                       TeamState seed1,
                       TeamState seed2)
{
    std::unordered_map<std::string, int> out;
    out.reserve(players.size());
    if (players.empty()) return out;

    std::vector<PlayerSlot> sorted = players;
    std::shuffle(sorted.begin(), sorted.end(), Rng());
    std::stable_sort(sorted.begin(), sorted.end(),
        [](const PlayerSlot& a, const PlayerSlot& b) {
            const float va = std::max(HealValue(a.profile_id, 1),
                                      HealValue(a.profile_id, 2));
            const float vb = std::max(HealValue(b.profile_id, 1),
                                      HealValue(b.profile_id, 2));
            return va > vb;
        });

    float heal1 = seed1.heal_score;
    int   size1 = seed1.size;
    float heal2 = seed2.heal_score;
    int   size2 = seed2.size;

    Logger::Log("team-balance",
        "[RoleWeightedSplit] ComputeBatchAssignment players=%zu seed1=(%.2f,%d) seed2=(%.2f,%d)\n",
        players.size(), seed1.heal_score, seed1.size,
        seed2.heal_score, seed2.size);

    for (const auto& p : sorted) {
        const int tf = GreedyPlaceOne(p.profile_id,
                                      heal1, size1, heal2, size2);
        if (tf == 1) {
            heal1 += HealValue(p.profile_id, 1);
            size1 += 1;
        } else {
            heal2 += HealValue(p.profile_id, 2);
            size2 += 1;
        }
        out[p.guid] = tf;
        Logger::Log("team-balance",
            "[RoleWeightedSplit]   guid=%s profile=%u -> tf=%d state=(%.2f,%d)/(%.2f,%d)\n",
            p.guid.c_str(), p.profile_id, tf,
            heal1, size1, heal2, size2);
    }

    return out;
}

}  // namespace RoleWeightedSplit
