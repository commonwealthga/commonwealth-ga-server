#pragma once
// Vendor purchase gates (instance side).
#include "sqlite3.h"
#include <cstdint>

namespace EconGate {
// Achievement an item (listed id or the item it creates) needs and this agent does
// not have yet; 0 = none needed / already earned. (asm_data_set_items.required_achievement_id,
// e.g. AvA HoloSuits 171-173, Holo dyes 368; client 1.5.1 shows the same requirement.)
inline int MissingVendorAchievement(sqlite3* db, int item_id, int grant_item, int64_t character_id) {
	if (!db) return 0;
	sqlite3_stmt* st = nullptr;
	int ach = 0;
	if (sqlite3_prepare_v2(db,
			"SELECT COALESCE(MAX(i.required_achievement_id), 0) FROM asm_data_set_items i "
			"WHERE i.item_id IN (?1, ?2) AND i.required_achievement_id > 0 AND NOT EXISTS "
			"(SELECT 1 FROM ga_character_achievements a WHERE a.character_id = ?3 AND a.achievement_id = i.required_achievement_id)",
			-1, &st, nullptr) != SQLITE_OK || !st) return 0;
	sqlite3_bind_int(st, 1, item_id);
	sqlite3_bind_int(st, 2, grant_item);
	sqlite3_bind_int64(st, 3, character_id);
	if (sqlite3_step(st) == SQLITE_ROW) ach = sqlite3_column_int(st, 0);
	sqlite3_finalize(st);
	return ach;
}
}  // namespace EconGate
