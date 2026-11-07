#pragma once
// Level-gated device slots, shared by the control server
// (equip_save -> DB, device list to the client) and the game DLL (pawn spawn +
// loadout switch). Data = the game's own GLOBAL UNLOCK GROUP
// (asm_data_set_device_slot_unlock_list, unlock type 1186 "Device Slot
// Unlock"; required_xp_level_id -> asm_data_set_xp_levels.level). Final build:
// ES2 Ranged lv2, ES7 Offhand 1 lv3, ES3 Specialty + ES8 Offhand 2 lv4,
// ES9 Offhand 3 + ES10 Morale lv5; everything else lv1.
// Equipped rows in a still-locked slot stay in the DB and simply light up
// when the agent reaches the level (starter kit rows included).

#include "sqlite3.h"
#include <cstdint>

namespace SlotUnlock {

// Tutorial chapter levels. The game DLL sets this to the
// level the current tutorial map's Kismet SetPlayerLevel starts at
// (Adrenaline 2, Skylark 3, Agency Zero 4; 0 elsewhere); CharacterLevel and
// the PRI level never go below it. Null in the control server.
inline int (*g_LevelFloor)() = nullptr;

// Player device slot unlock group (asm_data_set_bots.device_slot_unlock_group_id
// of the 4 player classes); replicated as TgPawn_Character.r_nDeviceSlotUnlockGrpId.
constexpr int kPlayerUnlockGroupId = 2;

// Engine equip point -> slot value id (FUN_109a1320, same table as
// GA::SlotValueId / EquipPointToSvid).
inline int EquipPointToSvid(int point) {
	static const int table[25] = {
		0,   221, 198, 200, 199, 201, 202, 203, 204, 385,
		386, 499, 500, 501, 502, 823, 996, 997, 998, 999,
		1000,1001,1002,1003,1004
	};
	return (point >= 1 && point <= 24) ? table[point] : 0;
}

// Agent level needed for a slot value id (1 = always open).
inline int RequiredLevelForSvid(sqlite3* db, int svid) {
	if (!db || svid <= 0) return 1;
	int level = 1;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT MAX(l.level) FROM asm_data_set_device_slot_unlock_list u "
	        "JOIN asm_data_set_xp_levels l ON l.xp_level_id = u.required_xp_level_id "
	        "WHERE u.device_slot_unlock_group_id = ? AND u.slot_value_id = ? "
	        "  AND u.unlock_type_value_id = 1186",
	        -1, &s, nullptr) == SQLITE_OK && s) {
		sqlite3_bind_int(s, 1, kPlayerUnlockGroupId);
		sqlite3_bind_int(s, 2, svid);
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL)
			level = sqlite3_column_int(s, 0);
		sqlite3_finalize(s);
	}
	return level < 1 ? 1 : level;
}

inline int CharacterLevel(sqlite3* db, int64_t character_id) {
	if (!db || character_id <= 0) return 1;
	int level = 1;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT COALESCE((SELECT MAX(l.level) FROM asm_data_set_xp_levels l "
	        "                 WHERE l.xp_value <= COALESCE(c.xp, 0)), 1) "
	        "FROM ga_characters c WHERE c.id = ?",
	        -1, &s, nullptr) == SQLITE_OK && s) {
		sqlite3_bind_int64(s, 1, character_id);
		if (sqlite3_step(s) == SQLITE_ROW) level = sqlite3_column_int(s, 0);
		sqlite3_finalize(s);
	}
	if (g_LevelFloor && g_LevelFloor() > level) level = g_LevelFloor();
	return level < 1 ? 1 : level;
}

// True when an agent of `level` may use engine equip point `point`.
inline bool IsEquipPointUnlocked(sqlite3* db, int level, int point) {
	return level >= RequiredLevelForSvid(db, EquipPointToSvid(point));
}

// Agent level an inventory item needs (asm_data_set_items.required_level of
// its base item; device rows store the item id in device_id). 1 when unknown.
// Rolled mods can raise the level the client shows - the base level is the
// floor we enforce server-side.
inline int RequiredLevelForItem(sqlite3* db, int device_id, int item_id) {
	const int id = item_id > 0 ? item_id : device_id;
	if (!db || id <= 0) return 1;
	int level = 1;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT MAX(required_level) FROM asm_data_set_items WHERE item_id = ?",
	        -1, &s, nullptr) == SQLITE_OK && s) {
		sqlite3_bind_int(s, 1, id);
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL)
			level = sqlite3_column_int(s, 0);
		sqlite3_finalize(s);
	}
	return level < 1 ? 1 : level;
}

// True when a comma-separated allowed_slots value lists `svid`.
inline bool CsvHas(const char* csv, int svid) {
	if (!csv) return false;
	for (const char* p = csv; *p; ) {
		while (*p == ',' || *p == ' ') ++p;
		if (!*p) break;
		int v = 0; bool any = false;
		while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); ++p; any = true; }
		if (any && v == svid) return true;
		if (!any) ++p;
	}
	return false;
}

inline bool IsItemUsable(sqlite3* db, int level, int device_id, int item_id) {
	return level >= RequiredLevelForItem(db, device_id, item_id);
}

} // namespace SlotUnlock
