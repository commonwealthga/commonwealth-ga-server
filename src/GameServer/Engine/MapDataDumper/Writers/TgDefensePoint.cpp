#include "src/GameServer/Engine/MapDataDumper/Writers/TgDefensePoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgDefensePoint(sqlite3* db, AActor* actor,
                         const std::string& mapName,
                         const std::string& className,
                         int mapObjectId) {
	auto* a = static_cast<ATgDefensePoint*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_defense_point ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" b_first_script, b_sniping, b_dont_change_position,"
		" b_avoid, b_disabled, b_not_in_vehicle, priority, num_checked"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_defense_point: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int   (stmt, 12, a->bFirstScript ? 1 : 0);
	sqlite3_bind_int   (stmt, 13, a->bSniping ? 1 : 0);
	sqlite3_bind_int   (stmt, 14, a->bDontChangePosition ? 1 : 0);
	sqlite3_bind_int   (stmt, 15, a->bAvoid ? 1 : 0);
	sqlite3_bind_int   (stmt, 16, a->bDisabled ? 1 : 0);
	sqlite3_bind_int   (stmt, 17, a->bNotInVehicle ? 1 : 0);
	sqlite3_bind_int   (stmt, 18, (int)a->Priority);
	sqlite3_bind_double(stmt, 19, a->NumChecked);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_defense_point: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
