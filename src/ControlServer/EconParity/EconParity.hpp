#pragma once
// Economy / progression rules from the original game.
// Small generic rules kept out of the shared big files (TcpSession / Database).
#include "src/ControlServer/Database/Database.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace EconParity {

// Character create. The creator's "Bald" builder asm 403 (and any id that is
// not a head / hair mesh in asm_data_set_assembly_meshes) crashes the pawn build in
// game; substitute the default head 1605 / hair 1974 (NewHair15) instead of failing.
void SanitizeCreateLooks(uint32_t& head_asm_id, uint32_t& hair_asm_id);

// Crafted mods (Weapon Mod 1434 / Armor Mod 1441). The mod item is generic
// (5984 = every Weapon Damage Mod); its effect is the BLUEPRINT's
// asm_data_set_blueprint_item_mods row per target quality (Uncommon 1164 /
// Rare 1163 / Epic 1162). After a craft the bag row keeps that list (as
// "quality:egid,...") in ga_crafted_mod_tiers, so the combine knows it.
void StampCraftedMod(int blueprint_id, const std::vector<int64_t>& inventory_ids);

struct CombineResult {
    bool ok = false;
    std::string reason;
    int64_t target_row = 0, mod_row = 0;
    int egid = 0;
};
// TgPlayerController.ServerCombineItems(nInvId, nModKitInvId) (Item Modification
// screen / profile wrench): one mod per item (a new one replaces the old one),
// Uncommon+ gear only, weapon mods on weapons, armor mods on armor, never on
// consumables; the target gets the egid for ITS quality (best tier the mod has
// at or below it); the mod row is spent. One DB transaction.
CombineResult CombineItems(int64_t user_id, int64_t character_id, int64_t target_row, int64_t mod_row);

// Booster effects the AP booster was still missing (wiki Booster; 1.37; 1.5):
// true when the agent's account booster (ga_users.booster_until) runs now.
bool BoosterActive(int64_t character_id);
// +2,000 tokens for every UTC midnight that passed while the account's booster ran
// (also while offline; per account, paid to the agent that is online; weekly/bank caps
// apply). Cheap to call every agent-info push. Returns tokens paid now.
int64_t BoosterNightlyTokens(int64_t character_id);
// Booster doubles end-of-mission loot: a 2nd roll of `loot_table_id` for every boosted
// agent of players_tf still in the instance. Returns rolls queued.
int QueueBoosterLoot(int64_t instance_id, int players_tf, int loot_table_id,
                     const std::vector<int64_t>& exclude);
// Booster: +50% assignment (quest) xp + credits.
void BoosterQuestBonus(int64_t character_id, int64_t& xp, int64_t& credits);

// Gear from the "Master Armor and Weapons - Uncommon/Rare/Epic" tables. The
// loot row's max_quality_value_id is the drop's quality (the item rows themselves
// are Common); the random stats (1.3 "weapons that drop as loot will have randomly-
// generated stats") = one effect group of the creating blueprint's mod for that
// quality, weighted by make_chance (e.g. 4 variants x 25%).
struct GearDrop { int item_id = 0, blueprint_id = 0, quality = 0; };
// Rolls `blueprint`'s stat variant for `quality` (0 = none in data).
int RollGearStats(int blueprint_id, int quality);
// 0957b: the item's full stat list = one RollGearStats per tier, Common (base mod, no letter) up to `quality`,
// as CSV in tier order ("" when the blueprint has no mods). Common quality = the base mod only.
std::string RollGearMods(int blueprint_id, int quality);
// After GrantLootItems: give each new row of a GearDrop item its quality + rolled stats.
void ApplyGearDrops(const std::vector<int64_t>& new_rows, const std::vector<GearDrop>& drops);
// 0957: same for a Database::RollLootTable auto-create roll (quest / achievement rewards).
void ApplyAutoGear(const std::vector<int64_t>& new_rows, const std::vector<Database::AutoGear>& gear);

// Lv50 XP -> credits (1.3 "Once players reach Level 50, they will now earn additional
// credits instead of accumulating additional XP"). One trigger: any xp written above the top
// level's xp is clamped back and paid as credits x cs_settings 'lv50_xp_credit_pct' (default 33
// = PvE win reward 300 xp : 100 cr; original rate unknown). Called once at startup.
void InitSchema();

}  // namespace EconParity
