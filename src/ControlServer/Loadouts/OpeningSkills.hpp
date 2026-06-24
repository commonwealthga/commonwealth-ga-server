#pragma once

#include <cstdint>
#include <vector>

#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"  // PROFILE_* constants

// =============================================================================
// Opening skill trees — the skill allocation a brand-new character gets across
// all 5 loadout profiles (item_profile_id 1..5), so new players don't have to
// figure out a build before they can play.
//
// Skill model (ga_character_skills):
//   - Each node is a (skill_group_id, skill_id) pair, allocated binary
//     (points = 1). Group 155 is common to every class; each class adds its
//     own groups (Medic 156/157, Assault 158/159, Recon 160/161,
//     Robotics 162/163).
//   - Budget is 13 nodes per profile. NOT enforced server-side — keep authored
//     builds at 13 (SeedOpeningSkills logs a warning otherwise).
//   - No level gating (characters have no level), so a fresh character may hold
//     a full build immediately.
//
// Authoring: skill nodes have no names in the data. Harvest IDs by building the
// allocation in-game on a test character, then reading ga_character_skills back
// from the DB. Paste the (group, skill) pairs here.
//
// Seeded once at creation by PlayerSessionStore::SeedOpeningSkills. Never
// re-run, so player edits are never clobbered.
// =============================================================================

namespace Loadouts {

struct SkillNode {
    int skill_group_id;
    int skill_id;
    int points = 1;  // binary today; field kept for any future multi-rank node
};

// Returns the skill build for (class profile_id, loadout_slot 1..5). Empty
// vector when the class/slot has no curated build (no skills seeded, no crash).
const std::vector<SkillNode>& GetOpeningSkills(uint32_t profile_id, int loadout_slot);

}  // namespace Loadouts
