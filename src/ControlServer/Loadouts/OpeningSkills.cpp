#include "src/ControlServer/Loadouts/OpeningSkills.hpp"

#include <array>

namespace Loadouts {

// =============================================================================
// Opening skill trees — 5 independent builds per class, one per loadout profile
// (item_profile_id 1..5). Profile N's skill tree pairs with profile N's
// equipped loadout (OpeningLoadouts.cpp) to form one coherent "Build N".
//
// Authoring: each per-class table has 5 slots; fill each with a 13-node build
// (budget is 13 per profile, NOT server-enforced — SeedOpeningSkills warns if a
// slot has some other count). Skill nodes have no names in the data: harvest
// (group, skill) IDs by building the allocation in-game on a test character,
// then reading ga_character_skills back from the DB.
//
// Only Medic profile 1 is authored as the worked example. Everything else is
// empty and waiting to be filled in.
// =============================================================================

// Per-class table: index [loadout_slot - 1] = the build for that profile.
using SkillTable = std::array<std::vector<SkillNode>, 5>;

static const SkillTable kMedicSkills = {{
    {
        { 155, 533 }, { 155, 676 }, { 155, 526 }, { 155, 550 }, { 155, 524 }, { 155, 525 },
        { 156, 651 }, { 156, 652 }, { 156, 801 }, { 156, 902 }, { 156, 904 }, { 156, 753 }, { 156, 703 },
    }, // BFB
    {
        { 155, 533 }, { 155, 676 }, { 155, 526 }, { 155, 545 }, { 155, 524 }, { 155, 525 },
        { 156, 651 }, { 156, 652 }, { 156, 804 }, { 156, 805 }, { 156, 902 }, { 156, 753 }, { 156, 703 },
    }, // Nanite
    {},
    {},
    {
        { 155, 553 }, { 155, 676 }, { 155, 526 }, { 155, 550 }, { 155, 524 }, { 155, 525 },
        { 157, 674 }, { 157, 760 }, { 157, 675 }, { 157, 806 }, { 157, 811 }, { 157, 810 }, { 157, 815 },
    }, // Pain Gun
}};

// TODO author the other classes (each 5 profiles wide); class groups are:
//   Robotics 162/163, Assault 158/159, Recon 160/161 — all plus common 155.
// static const SkillTable kRoboticsSkills = {{ {...}, {}, {}, {}, {} }};
// static const SkillTable kAssaultSkills  = {{ {...}, {}, {}, {}, {} }};
// static const SkillTable kReconSkills    = {{ {...}, {}, {}, {}, {} }};

static const std::vector<SkillNode> kEmpty = {};

const std::vector<SkillNode>& GetOpeningSkills(uint32_t profile_id, int loadout_slot) {
    if (loadout_slot < 1 || loadout_slot > 5) return kEmpty;
    switch (profile_id) {
        case PROFILE_MEDIC:    return kMedicSkills[loadout_slot - 1];
        // case PROFILE_ROBOTICS: return kRoboticsSkills[loadout_slot - 1];
        // case PROFILE_ASSAULT:  return kAssaultSkills[loadout_slot - 1];
        // case PROFILE_RECON:    return kReconSkills[loadout_slot - 1];
        default:               return kEmpty;
    }
}

}  // namespace Loadouts
