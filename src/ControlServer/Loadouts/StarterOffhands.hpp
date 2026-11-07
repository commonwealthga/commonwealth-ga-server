#pragma once

#include <cstdint>
#include <vector>

#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"

// The original level 1-5 starter kit unlocks THREE class
// offhands (wiki "Level"; 1.3 patch notes: new medics get the Poison
// Grenade). The game data has exactly three offhands per class at required
// level 3/4/5. The per-slot kit pick (lowest level per equip slot) loses one
// or two of them, and some are not in the upstream loadout list at all, so
// the starter kit's offhand slots are rebuilt from this fixed list.
namespace StarterOffhands {

// Replace every offhand entry (SVID 203/204/385) of `kit` with the class's
// three data offhands whose required level <= max_level, as COMMON with no
// rolled mods. Unknown profile ids leave `kit` unchanged.
void Apply(uint32_t profile_id, int max_level, std::vector<Loadouts::GearSlot>& kit);

}  // namespace StarterOffhands
