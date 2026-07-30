#include "src/GameServer/Engine/ObjectiveNames/ObjectiveNames.hpp"
#include "src/Database/Database.hpp"

#include <unordered_map>

namespace ObjectiveNames {

std::string LookupFriendlyName(int objectiveId) {
	static std::unordered_map<int, std::string> cache;

	auto it = cache.find(objectiveId);
	if (it != cache.end()) return it->second;

	std::string name;

	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		const char* kSql =
			"SELECT m.message "
			"FROM asm_data_set_objectives o "
			"JOIN asm_data_set_msg_translations m ON o.text_msg_id = m.msg_id "
			"WHERE o.objective_id = ? "
			"LIMIT 1";
		// Prepare failure = table(s) missing on this DB (pre-import dev copy) --
		// cache the empty result same as a real "not found" so we don't retry
		// the failing prepare on every subsequent lookup for this id.
		if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) == SQLITE_OK && stmt) {
			sqlite3_bind_int(stmt, 1, objectiveId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				const unsigned char* text = sqlite3_column_text(stmt, 0);
				if (text) name = reinterpret_cast<const char*>(text);
			}
			sqlite3_finalize(stmt);
		}
	}

	cache[objectiveId] = name;
	return name;
}

}  // namespace ObjectiveNames
