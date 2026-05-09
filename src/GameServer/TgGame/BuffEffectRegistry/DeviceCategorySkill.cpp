#include "src/GameServer/TgGame/BuffEffectRegistry/DeviceCategorySkill.hpp"

#include <map>

#include "sqlite3.h"

#include "src/Database/Database.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace DeviceCategorySkill {

namespace {

std::map<int, int> g_deviceToSkill;  // deviceId → category skill_id
bool g_inited = false;

void DoInit() {
	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("db",
			"[DeviceCategorySkill] no DB connection — lookups will return 0\n");
		return;
	}

	// Walk every asm_data_set_items row where the item *is* the device
	// (i.e. item_id == device_id, with skill_id non-zero so we don't insert
	// noise rows). The convention: when an item entry's `ref_device_id` is 0
	// AND a row in `asm_data_set_devices` exists for the same numeric id,
	// the item-id and device-id refer to the same entity.
	const char* sql =
	    "SELECT i.item_id, i.skill_id "
	    "FROM asm_data_set_items i "
	    "JOIN asm_data_set_devices d ON d.device_id = i.item_id "
	    "WHERE i.skill_id > 0";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("db",
			"[DeviceCategorySkill] prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}

	int rows = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int deviceId = sqlite3_column_int(stmt, 0);
		int skillId  = sqlite3_column_int(stmt, 1);
		g_deviceToSkill[deviceId] = skillId;
		++rows;
	}
	sqlite3_finalize(stmt);

	Logger::Log("db",
		"[DeviceCategorySkill] indexed %d device→skillId mappings\n", rows);
}

}  // namespace

int Lookup(int deviceId) {
	if (!g_inited) { DoInit(); g_inited = true; }
	auto it = g_deviceToSkill.find(deviceId);
	return (it != g_deviceToSkill.end()) ? it->second : 0;
}

int LookupByInstanceId(ATgPawn* pawn, int deviceInstanceId) {
	if (!pawn || deviceInstanceId == 0) return 0;
	const auto& equipped = Inventory::GetEquipped(pawn);
	for (const EquippedEntry& e : equipped) {
		if (e.inventoryId == deviceInstanceId) {
			return Lookup(e.deviceId);
		}
	}
	return 0;
}

}  // namespace DeviceCategorySkill
