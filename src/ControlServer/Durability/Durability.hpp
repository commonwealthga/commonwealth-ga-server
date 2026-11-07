#pragma once
// Weapon + armour durability (original 1.1-1.39 rules).
//
// What the original game did (patch notes 1.1 / 1.3 / 1.35 / 1.37 / 1.39, the
// game's own texts 42341-42347 / 55777 / 56050 and the archived wiki page
// "Repair Kit"):
//   - Uncommon, Rare and Epic weapons and armour have durability 0..max; every
//     mission the piece is used in takes 1 point off.
//   - At 0 the item acts as a Common item: base values stay, the upgraded
//     stats (its mods) do not apply until it is repaired.
//   - A Repair Kit of the SAME quality (5467 Uncommon / 5468 Rare / 5469 Epic)
//     restores one item to full and is used up. Common items never wear.
//
// How the client shows it: SEND_INVENTORY field DURABILITY (0x220) is the
// current value, the max is the durability of the blueprint sent in
// BLUEPRINT_ID (asm_data_set_blueprints.durability). The Agent Profile wrench
// "Repair" button checks for a same-quality kit itself and then calls the
// TgPlayerController.ServerRepairInvItem RPC (game instance -> GAME_EVENT
// "repair_item" -> Repair() here).
//
// Storage: ga_players_inventory.durability, -1 = full (every existing row
// starts full). cs_settings: durability_enabled ('1' default, '0' = no wear
// and everything shows full), durability_wear_per_mission (default 1).
#include <cstdint>
#include <string>
#include <vector>

namespace Durability {

void EnsureSchema();

// Value for the DURABILITY field of one player inventory record.
// blueprint_id = the BLUEPRINT_ID sent in the same record.
int ForWire(int64_t inventory_id, int blueprint_id);

// Mission end: every agent of the instance loses wear points on the equipped
// (active profile) Uncommon+ gear that has upgrades. Returns rows changed.
int WearForInstance(int64_t instance_id);

struct RepairResult {
    bool ok = false;
    int64_t kit_row = 0;
    int64_t target_row = 0;
    std::string reason;
};
RepairResult Repair(int64_t user_id, int64_t character_id, int64_t target_row);

}  // namespace Durability
