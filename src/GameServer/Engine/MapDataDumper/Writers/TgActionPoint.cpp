#include "src/GameServer/Engine/MapDataDumper/Writers/TgActionPoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgActionPoint(sqlite3* db, AActor* actor,
                        const std::string& mapName,
                        const std::string& className,
                        int mapObjectId) {
	auto* a = static_cast<ATgActionPoint*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_action_point ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" action_type, n_objective_num, n_task_force, b_use_rotation"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_action_point: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, (int)a->ActionType);
	sqlite3_bind_int(stmt, 13, (int)a->nObjectiveNum);
	sqlite3_bind_int(stmt, 14, (int)a->nTaskForce);
	sqlite3_bind_int(stmt, 15, a->bUseRotation ? 1 : 0);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_action_point: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
