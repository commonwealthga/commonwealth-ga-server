#include "src/GameServer/Engine/MapDataDumper/Writers/TgFlagCaptureVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgFlagCaptureVolume(sqlite3* db, AActor* actor,
                              const std::string& mapName,
                              const std::string& className,
                              int mapObjectId) {
	auto* a = static_cast<ATgFlagCaptureVolume*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_flag_capture_volume ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" r_n_task_force, r_e_coalition"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_flag_capture_volume: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, (int)a->r_nTaskForce);
	sqlite3_bind_int(stmt, 13, (int)a->r_eCoalition);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_flag_capture_volume: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
