#pragma once

// Varied looks + names for class bots (no two bots alike).
// Pools come from the game's own item tables (asm_data_set_items +
// asm_data_set_item_mesh_asm_groups + asm_data_set_assembly_meshes): whole
// "Suit - <Class> <Set>" + "Helm - <Class> <Set>" pairs, the class HoloSuit /
// Commonwealth suits (rarer, bare head), tier 1-5 dyes, jetpack trails, the
// character-builder heads + hair. Weights and the name list can be overridden
// by an optional pvp-bot-looks.json in the working directory.

#include <string>

class ATgPawn;

namespace BotLooks {

struct Look {
    int gender = 0;      // 853 male, 852 female
    int head = 0;        // asm id
    int hair = 0;        // asm id
    int suit = 0;        // item id (Suit/HoloSuit cosmetic)
    int helmet = 0;      // item id, 0 = bare head
    int dye1 = 0;        // primary dye item (0 = none)
    int dye2 = 0;        // secondary dye item
    int trail = 0;       // jetpack trail item (0 = none)
    std::wstring name;   // "[BOT] <callsign>"
};

// Look of the bot that owns `key` (respawn slot). key 0 = a fresh look under a
// new temporary key (see LastTempKey). Same key -> same look + name.
const Look& ForKey(int key, const std::string& cls);
int  LastTempKey();
void Rekey(int fromKey, int toKey);
void Release(int key);       // frees the name for reuse

// Writes body, suit, helmet, dyes, trail and the name onto a bot pawn.
// Returns true if the suit applied (pawn became a player-look body).
bool Apply(ATgPawn* pawn, const Look& look);

}  // namespace BotLooks
