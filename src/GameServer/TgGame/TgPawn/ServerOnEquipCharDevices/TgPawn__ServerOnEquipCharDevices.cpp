#include "src/GameServer/TgGame/TgPawn/ServerOnEquipCharDevices/TgPawn__ServerOnEquipCharDevices.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <map>
#include <vector>
#include <cstdlib>

// A tutorial grant resolved to one of the agent's own bag rows: the client
// only binds a device whose inventory id it got in SEND_INVENTORY, so a fresh
// Inventory::NextId() would equip server-side but stay invisible/unusable.
struct TutorialGrant {
	int deviceId;
	int inventoryId;
	int quality;
	std::vector<int> mods;
};

// characterId -> (slot -> grant) handed out by the tutorial in this instance.
static std::map<int, std::map<int, TutorialGrant>> s_tutorialGrants;

bool TgPawn__ServerOnEquipCharDevices::IsTutorialStripSlot(int slot) {
	// weapons, travel, offhands, morale, consumable; keeps suit (6), beacon
	// (11), helmet (12) and the class device (14).
	switch (slot) {
		case 1: case 2: case 3: case 4: case 5:
		case 7: case 8: case 9: case 10: case 13:
			return true;
		default:
			return false;
	}
}

// Engine slot for a device from its asm_data_set_devices category.
static int SlotForDevice(int deviceId) {
	int category = 0;
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (db && sqlite3_prepare_v2(db,
			"SELECT slot_used_value_id FROM asm_data_set_devices WHERE device_id = ? LIMIT 1",
			-1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, deviceId);
		if (sqlite3_step(stmt) == SQLITE_ROW) category = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
	}
	switch (category) {
		case 389: return 1;   // melee
		case 388: return 2;   // ranged
		case 806: return 5;   // jetpack
		case 476: return 10;  // morale
		case 390: return 7;   // off-hand
		default:  return 0;
	}
}

static std::vector<int> ParseMods(const unsigned char* csv) {
	std::vector<int> mods;
	const char* p = reinterpret_cast<const char*>(csv);
	while (p && *p) {
		while (*p == ',' || *p == ' ') ++p;
		if (!*p) break;
		char* end = nullptr;
		long v = std::strtol(p, &end, 10);
		if (end == p) break;
		mods.push_back((int)v);
		p = end;
	}
	return mods;
}

// The exact device if the agent owns it (starter kit has the class jetpack),
// else the agent's own equipped item for that slot (their kit melee).
static bool ResolveGrant(int charId, int deviceId, int slot, TutorialGrant& out) {
	static const char* kQueries[2] = {
		"SELECT i.id, i.device_id, i.quality, i.mod_effect_group_ids "
		"FROM ga_players_inventory i JOIN ga_characters c ON c.user_id = i.user_id "
		"WHERE c.id = ?1 AND i.device_id = ?2 ORDER BY i.id LIMIT 1",
		"SELECT i.id, i.device_id, i.quality, i.mod_effect_group_ids "
		"FROM ga_character_devices d JOIN ga_players_inventory i ON i.id = d.inventory_id "
		"WHERE d.character_id = ?1 AND d.equipped_slot = ?3 AND i.device_id > 0 "
		"ORDER BY d.item_profile_id LIMIT 1",
	};
	sqlite3* db = Database::GetConnection();
	if (!db) return false;
	for (const char* sql : kQueries) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) continue;
		sqlite3_bind_int(stmt, 1, charId);
		sqlite3_bind_int(stmt, 2, deviceId);
		sqlite3_bind_int(stmt, 3, slot);
		bool found = false;
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			out.inventoryId = sqlite3_column_int(stmt, 0);
			out.deviceId    = sqlite3_column_int(stmt, 1);
			out.quality     = sqlite3_column_int(stmt, 2);
			out.mods        = ParseMods(sqlite3_column_text(stmt, 3));
			found = true;
		}
		sqlite3_finalize(stmt);
		if (found) return true;
	}
	return false;
}

static void EquipGrant(ATgPawn* Pawn, int slot, const TutorialGrant& g) {
	for (const EquippedEntry& e : Inventory::GetEquipped(Pawn)) {
		if (e.slot == slot && e.inventoryId == g.inventoryId) return;  // already held
	}
	Inventory::Unequip(Pawn, slot);
	Inventory::Equip(Pawn, g.deviceId, slot, g.quality, g.inventoryId, g.mods);
}

void TgPawn__ServerOnEquipCharDevices::ReplayTutorialGrants(ATgPawn* Pawn) {
	if (!Pawn) return;
	const int charId = ((ATgPawn_Character*)Pawn)->s_nCharacterId;
	auto it = s_tutorialGrants.find(charId);
	if (it == s_tutorialGrants.end()) return;
	for (const auto& g : it->second) {
		Inventory::Equip(Pawn, g.second.deviceId, g.first, g.second.quality,
			g.second.inventoryId, g.second.mods);
		Logger::Log("kismet", "[Tutorial] replay grant char=%d slot=%d device=%d inv=%d\n",
			charId, g.first, g.second.deviceId, g.second.inventoryId);
	}
}

void __fastcall TgPawn__ServerOnEquipCharDevices::Call(ATgPawn* Pawn, void* edx, UTgSeqAct_EquipCharDevices* Action) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Action);
	if (!Pawn || !Action || Pawn->bDeleteMe) {
		LogCallEnd();
		return;
	}
	const int charId = ((ATgPawn_Character*)Pawn)->s_nCharacterId;
	bool changed = false;

	if (Action->m_bRemoveOtherDevices) {
		std::vector<int> strip;
		for (const EquippedEntry& e : Inventory::GetEquipped(Pawn)) {
			if (IsTutorialStripSlot(e.slot)) strip.push_back(e.slot);
		}
		for (int slot : strip) Inventory::Unequip(Pawn, slot);
		s_tutorialGrants.erase(charId);
		changed = changed || !strip.empty();
		Logger::Log("kismet", "[Tutorial] strip char=%d removed=%zu\n", charId, strip.size());
	}

	for (int i = 0; i < Action->m_DeviceIdList.Num(); i++) {
		const int deviceId = Action->m_DeviceIdList(i);
		const int slot = SlotForDevice(deviceId);
		if (deviceId <= 0 || slot == 0) {
			Logger::Log("kismet", "[Tutorial] grant char=%d device=%d: no slot, skipped\n", charId, deviceId);
			continue;
		}
		TutorialGrant g;
		if (!ResolveGrant(charId, deviceId, slot, g)) {
			Logger::Log("kismet", "[Tutorial] grant char=%d device=%d slot=%d: nothing in the bag, skipped\n",
				charId, deviceId, slot);
			continue;
		}
		EquipGrant(Pawn, slot, g);
		s_tutorialGrants[charId][slot] = g;
		changed = true;
		Logger::Log("kismet", "[Tutorial] grant char=%d slot=%d device=%d -> bag device=%d inv=%d\n",
			charId, slot, deviceId, g.deviceId, g.inventoryId);
	}

	if (changed) Inventory::Finalize(Pawn);
	LogCallEnd();
}
