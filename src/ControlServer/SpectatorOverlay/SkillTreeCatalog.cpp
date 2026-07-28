#include "src/ControlServer/SpectatorOverlay/SkillTreeCatalog.hpp"

#include <ctime>
#include <mutex>
#include <unordered_map>

#include "src/ControlServer/Constants/GameTypes.h"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"

namespace SkillTreeCatalog {

namespace {

constexpr int kBalancedGroupId = 155;

// Defensive backstop only -- see GetCachedBuildSummary's doc comment. Real
// invalidation is event-driven (InvalidateCache called from TcpSession's
// profile_switch/skill_save/skill_respec handlers), so this just bounds
// the damage if some future code path changes ga_character_skills or the
// active item_profile_id without remembering to call InvalidateCache.
constexpr int64_t kStaleSeconds = 30;

struct CacheEntry {
    BuildSummary summary;
    int64_t computed_at = 0;
};

std::mutex g_cache_mutex;
std::unordered_map<int64_t, CacheEntry> g_cache;

struct ClassTrees {
    int tree1_group;
    int tree2_group;
    const char* tree1_name;
    const char* tree2_name;
};

// Confirmed against asm_data_set_skill_group_set_groups / asm_data_set_skill_groups.
const ClassTrees* TreesForProfile(uint32_t profile_id) {
    static const ClassTrees kMedic    = {156, 157, "Healer",       "Poison"};
    static const ClassTrees kAssault  = {158, 159, "Tank",         "Destroyer"};
    static const ClassTrees kRecon    = {160, 161, "Infiltration", "Marksman"};
    static const ClassTrees kRobotic  = {162, 163, "Engineer",     "Drones"};
    switch (profile_id) {
        case GA_G::PROFILE_ID_MEDIC:   return &kMedic;
        case GA_G::PROFILE_ID_ASSAULT: return &kAssault;
        case GA_G::PROFILE_ID_RECON:   return &kRecon;
        case GA_G::PROFILE_ID_ROBOTIC: return &kRobotic;
        default: return nullptr;
    }
}

} // namespace

BuildSummary GetBuildSummary(int64_t character_id, uint32_t profile_id) {
    BuildSummary out;
    if (character_id == 0) return out;

    const ClassTrees* trees = TreesForProfile(profile_id);
    if (trees) {
        out.tree1_name = trees->tree1_name;
        out.tree2_name = trees->tree2_name;
    }

    const int item_profile_id = PlayerSessionStore::GetCurrentItemProfile(character_id);
    for (const auto& row : PlayerSessionStore::GetSkillsForCharacter(character_id, item_profile_id)) {
        if (row.skill_group_id == kBalancedGroupId) {
            out.balanced += row.points;
        } else if (trees && row.skill_group_id == trees->tree1_group) {
            out.tree1 += row.points;
        } else if (trees && row.skill_group_id == trees->tree2_group) {
            out.tree2 += row.points;
        }
        // Anything else (device-list/base-attribute groups) isn't part of
        // the 3-tree build notation -- ignored.
    }
    return out;
}

BuildSummary GetCachedBuildSummary(int64_t character_id, uint32_t profile_id) {
    if (character_id == 0) return BuildSummary{};

    const int64_t now = (int64_t)std::time(nullptr);
    std::lock_guard<std::mutex> lock(g_cache_mutex);

    auto it = g_cache.find(character_id);
    if (it != g_cache.end() && (now - it->second.computed_at) < kStaleSeconds) {
        return it->second.summary;
    }

    BuildSummary summary = GetBuildSummary(character_id, profile_id);
    g_cache[character_id] = CacheEntry{summary, now};
    return summary;
}

void InvalidateCache(int64_t character_id) {
    std::lock_guard<std::mutex> lock(g_cache_mutex);
    g_cache.erase(character_id);
}

}  // namespace SkillTreeCatalog
