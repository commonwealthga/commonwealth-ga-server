#pragma once

#include <cstdint>

// Combat Rating stars (SKILL_RATING, 0..5,
// half steps) from PvP results only, like the original (wiki Combat_Rating:
// "only affected by performance in PvP missions", new agents 0 stars, "upon
// reaching Lv30, an agent gains 2 stars"). Source = the active MMR engine's
// rating for the agent's class (MmrService, Elo start 1000). No original
// formula survives: unrated = 0; rated = 2.5 + (rating - 1000) / per_star,
// per_star = cs_settings 'combat_rating_per_star' (default 100) -> 1000 = 2.5,
// 1250 = 5, 750 = 0. Level 30+ never shows less than 2.
namespace CombatRating {

float Stars(int64_t character_id, uint32_t level);

}  // namespace CombatRating
