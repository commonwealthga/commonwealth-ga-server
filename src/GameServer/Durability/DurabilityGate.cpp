#include "src/GameServer/Durability/DurabilityGate.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace DurabilityGate {

bool Broken(int inventoryId) {
	if (inventoryId <= 0) return false;
	sqlite3* db = Database::GetConnection();
	if (!db) return false;
	sqlite3_stmt* s = nullptr;
	// Prepare fails on a DB without the column (old node seed) -> not worn.
	if (sqlite3_prepare_v2(db, "SELECT durability FROM ga_players_inventory WHERE id = ?", -1, &s, nullptr) != SQLITE_OK)
		return false;
	sqlite3_bind_int(s, 1, inventoryId);
	bool broken = false;
	if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL)
		broken = sqlite3_column_int(s, 0) == 0;
	sqlite3_finalize(s);
	if (broken && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'durability_enabled'", -1, &s, nullptr) == SQLITE_OK) {
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL && sqlite3_column_int(s, 0) == 0)
			broken = false;  // switched off: nothing counts as worn
		sqlite3_finalize(s);
	}
	if (broken) Logger::Log("inventory", "[Durability] inv=%d worn out (0) - upgraded stats off until repaired\n", inventoryId);
	return broken;
}

std::vector<int> Gate(int inventoryId, const std::vector<int>& mods) {
	if (mods.empty() || !Broken(inventoryId)) return mods;
	return {};
}

}  // namespace DurabilityGate
