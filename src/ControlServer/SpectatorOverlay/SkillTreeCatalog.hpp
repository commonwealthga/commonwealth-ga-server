#pragma once

#include <cstdint>

// Maps a character's ga_character_skills rows (skill_group_id, skill_id,
// points) onto the "P/T1/T2" build notation players use in chat (e.g.
// "6/0/7" -- points in Balanced, their class's first tree, second tree).
//
// skill_group_id is fixed game data confirmed directly against the DB
// (asm_data_set_skill_group_set_groups joined with asm_data_set_skill_groups):
//   155 = Balanced (shared -- every class's skill_group_set includes it)
//   156/157 = MEDIC    Healer/Poison
//   158/159 = ASSAULT  Tank/Destroyer
//   160/161 = RECON    Infiltration/Marksman
//   162/163 = ROBOTIC  Engineer/Drones
// Each skill_group_set also has a "- X DEVICE LIST" and "- BASE ATTRIBUTES"
// group (123, 164-167) -- not a build tree, deliberately excluded here.
//
// This is NOT the same id space as OverlayIdCatalog's effect/skill ids
// (UTgEffectGroup ids granted BY a skill point) -- skill_group_id/skill_id
// here identify the build-tree NODE itself, from ga_character_skills /
// asm_data_set_skill_group_skills.
namespace SkillTreeCatalog {

struct BuildSummary {
    int balanced = 0;
    int tree1 = 0;
    int tree2 = 0;
    const char* tree1_name = "";
    const char* tree2_name = "";
};

// Sums invested points into Balanced/Tree1/Tree2 buckets for the character
// currently active on session_guid's connection. Resolves the active
// loadout (PlayerSessionStore::GetCurrentItemProfile) and profile_id
// (class) internally. Returns all-zero (empty tree names) if character_id
// is 0 or unresolvable. Always hits the DB -- see GetCachedBuildSummary for
// the version the overlay endpoints should actually call.
BuildSummary GetBuildSummary(int64_t character_id, uint32_t profile_id);

// Same result as GetBuildSummary, but served from a per-character cache
// instead of hitting ga_character_skills on every call. There is no
// mid-match respec (confirmed -- players can only swap between their 5
// pre-built loadout profiles, never reallocate points, while in a match),
// so ga_character_skills itself is effectively static for the lifetime of
// a cache entry. The one thing that DOES change mid-match is which
// item_profile_id is active, and that's already event-driven via the
// "profile_switch" IPC event (TcpSession.cpp) -- InvalidateCache is called
// from there, so a profile swap propagates on the NEXT overlay poll after
// the swap completes, not after some blind timeout.
// kStaleSeconds (see .cpp) is a defensive backstop only, for any
// invalidation path not yet wired up -- not the primary mechanism.
BuildSummary GetCachedBuildSummary(int64_t character_id, uint32_t profile_id);

// Drops character_id's cached entry, if any, so the next
// GetCachedBuildSummary call recomputes from the DB. Call this wherever
// ga_character_skills or the active item_profile_id changes for a
// character: the "profile_switch", "skill_save", and "skill_respec" IPC
// event handlers in TcpSession.cpp.
void InvalidateCache(int64_t character_id);

}  // namespace SkillTreeCatalog
