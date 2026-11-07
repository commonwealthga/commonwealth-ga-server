#pragma once
// Original 1.3 rule - a fully worn item (durability 0)
// keeps its base values but its upgraded stats (mods) do not apply until it
// is repaired. Durability lives in ga_players_inventory.durability (control
// server, -1 = full); a DB without the column = nothing is worn.
#include <vector>

namespace DurabilityGate {
bool Broken(int inventoryId);
// mods, or nothing when the item is worn out.
std::vector<int> Gate(int inventoryId, const std::vector<int>& mods);
}
