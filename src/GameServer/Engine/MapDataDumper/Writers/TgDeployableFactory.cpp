#include "src/GameServer/Engine/MapDataDumper/Writers/TgDeployableFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgDeployableFactory(sqlite3* db, AActor* actor,
                              const std::string& mapName,
                              const std::string& className,
                              int mapObjectId) {
	auto* a = static_cast<ATgDeployableFactory*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_deployable_factory ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" n_current_count, s_f_last_spawn_time, s_b_spawn_once"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_deployable_factory: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int   (stmt, 12, a->nCurrentCount);
	sqlite3_bind_double(stmt, 13, a->s_fLastSpawnTime);
	sqlite3_bind_int   (stmt, 14, a->s_bSpawnOnce ? 1 : 0);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_deployable_factory: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
