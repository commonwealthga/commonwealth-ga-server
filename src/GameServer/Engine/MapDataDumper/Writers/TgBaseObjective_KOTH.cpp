#include "src/GameServer/Engine/MapDataDumper/Writers/TgBaseObjective_KOTH.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgBaseObjective_KOTH(sqlite3* db, AActor* actor,
                               const std::string& mapName,
                               const std::string& className,
                               int mapObjectId) {
	auto* a = static_cast<ATgBaseObjective_KOTH*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_base_objective_koth ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" c_f_prev_client_capt_time"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_base_objective_koth: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_double(stmt, 12, a->c_fPrevClientCaptTime);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_base_objective_koth: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
