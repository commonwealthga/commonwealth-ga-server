#include "src/GameServer/Engine/MapGameInfo/MapGameInfo.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

namespace {

std::string ReservedMapName(const std::string& mapName) {
	return mapName + "_reserved";
}

}

std::optional<MapGameInfoRow> MapGameInfo::LookupByNameAndGameMode(
		const std::string& mapName, const std::string& gameMode) {
	if (mapName.empty()) return std::nullopt;

	sqlite3* db = Database::GetConnection();
	if (!db) return std::nullopt;

	sqlite3_stmt* stmt = nullptr;
	// Two maps can share a map_name across game classes (stock CTR row + custom
	// PointRotation row). Order so the row whose game_class matches gameMode wins;
	// with an empty/unmatched gameMode the predicate is 0 for every row and this
	// degrades to "first name match" — the old name-only behavior.
	const char* kSql =
		"SELECT map_game_id, mission_time_secs, is_pvp, overtime_secs, allow_overtime "
		"FROM map_game_info "
		"WHERE map_name = ? COLLATE NOCASE OR map_name = ? COLLATE NOCASE "
		"ORDER BY (game_class = ? COLLATE NOCASE) DESC "
		"LIMIT 1";
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		// Pre-v95/v97 DB (column missing). Treat as "no override" so callers
		// fall back to their hardcoded defaults — same behavior as before.
		Logger::Log("config", "MapGameInfo::LookupByNameAndGameMode prepare failed: %s\n", sqlite3_errmsg(db));
		return std::nullopt;
	}
	sqlite3_bind_text(stmt, 1, mapName.c_str(), -1, SQLITE_TRANSIENT);
	const std::string reservedMapName = ReservedMapName(mapName);
	sqlite3_bind_text(stmt, 2, reservedMapName.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, gameMode.c_str(), -1, SQLITE_TRANSIENT);

	std::optional<MapGameInfoRow> out;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		MapGameInfoRow r;
		r.map_game_id       = sqlite3_column_int(stmt, 0);
		r.mission_time_secs = sqlite3_column_int(stmt, 1);
		r.is_pvp            = sqlite3_column_int(stmt, 2) != 0;
		r.overtime_secs     = sqlite3_column_int(stmt, 3);
		r.allow_overtime    = sqlite3_column_int(stmt, 4) != 0;
		out = r;
	}
	sqlite3_finalize(stmt);
	return out;
}

std::optional<MapGameInfoRow> MapGameInfo::LookupByName(const std::string& mapName) {
	return LookupByNameAndGameMode(mapName, "");
}
