#pragma once

#include "lib/sqlite3/sqlite3.h"
#include <cstdint>
#include <string>
#include <vector>

class Database {
private:
	static sqlite3* connection;
public:
	static int Callback(void* data, int argc, char** argv, char** azColName);
	static sqlite3* GetConnection();
	static void CloseConnection();
	static void Init();

	// Agency name for a character, "" when unaffiliated. Feeds the in-match
	// scoreboard's Agency column (ATgRepInfo_Player::r_sAgencyName).
	static std::string GetAgencyName(int64_t character_id);
	// ga_characters.xp -> level via asm_data_set_xp_levels (1 if unknown).
	static int GetCharacterLevel(int64_t character_id);
	// Both characters in the same squad (ga_squad_members, kept by the control server).
	static bool SameSquad(int64_t character_a, int64_t character_b);
	// asm_data_set_quest_group_lists rows (quest + status 1618 Completed / 1619 Active /
	// 1620 Active or Completed / 1621 Incomplete) all met (any_row: at least one) from
	// ga_character_quests. Rows without a status are ignored; no status rows = open.
	static bool QuestGroupSatisfied(int64_t character_id, int quest_group_id, bool any_row = false);
	// A Map Transition volume's travel condition: its own quest group (any row), and
	// 0988: the Dome City north door (262) also opens on the Sonoran door's group (47).
	static bool MapDoorOpen(int64_t character_id, int ui_volume_id, int quest_group_id);
	// 0988: quest group 47 ("The Price of Liberty" complete or "Seeking the Cure" taken).
	static bool DomeExitCleared(int64_t character_id);
	struct CharacterWallet { int credits = 0; int tokens = 0; int xp = 0; int ap = 0; };  // ap = account Agenda Points
	static CharacterWallet GetCharacterWallet(int64_t character_id);
	static int GetBoosterSecondsLeft(int64_t character_id);
	// Bag row inv_id owned by the character's account and a vanity pet
	// (item type 1535) -> its bot id (asm items.ref_bot_id), item id in *out_item_id; else 0.
	static int GetVanityPetBotId(int64_t character_id, int64_t inv_id, int* out_item_id);
	static void SetActivePetInvId(int64_t character_id, int64_t inv_id);
	static int64_t GetActivePetInvId(int64_t character_id);
	// Vendor buy: price from the game tables (never the client's),
	// wallet check + deduct + ga_players_inventory row(s) in one transaction.
	struct VendorPurchaseResult { bool ok = false; std::string reason; int item_id = 0; int price = 0; int count = 0; bool component = false; std::vector<int64_t> inventory_ids; CharacterWallet wallet; int discount = 0; /* 0535 */ };
	static VendorPurchaseResult VendorPurchase(int64_t user_id, int64_t character_id, int loot_table_id,
		int loot_table_item_id, int count, int currency_type);
	// PvE kill reward: the dead bot's xp_value / currency_value
	// (asm_data_set_bots), added to the agent (xp capped at the top of the curve).
	// Loot drops are upstream's credit_loot_table (quest_kill_credit event).
	struct InvRemoveResult { bool ok = false; std::string reason; int item_id = 0; int value = 0; int currency = 0; std::vector<int64_t> inventory_ids; CharacterWallet wallet; };
	static InvRemoveResult DestroyInvItem(int64_t user_id, int64_t character_id, int64_t inventory_id, bool sell);
	// A used consumable - drop the bag row and any equip rows pointing at it.
	static bool ConsumeInvItem(int64_t user_id, int64_t inventory_id);
	// paid_xp / paid_credits: what the agent actually ends up with once the level 50 cap
	// (trg_lv50_xp_credits: overflow XP -> lv50_xp_credit_pct % credits) is applied.
	struct BotKillReward { bool ok = false; int xp = 0; int credits = 0; int paid_xp = 0; int paid_credits = 0; CharacterWallet wallet; };
	static BotKillReward AwardKillXp(int64_t user_id, int64_t character_id, int bot_id);

	// Returns "" (not found), "active", or "complete"
	static std::string GetQuestStatus(int64_t character_id, int quest_id);
	static void AcceptQuest(int64_t character_id, int quest_id);
	static void CompleteQuest(int64_t character_id, int quest_id);
	static void AbandonQuest(int64_t character_id, int quest_id);
};

