#include "src/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryPatrolPath.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgBotFactoryPatrolPath(sqlite3* db, ATgBotFactory* factory,
                                 const std::string& mapName,
                                 int mapObjectId) {
	if (!factory || factory->PatrolPath.Data == nullptr) return;

	const char* kSql =
		"INSERT INTO map_tg_bot_factory_patrol_path ("
		"map_name, map_object_id, array_index,"
		" location_x, location_y, location_z, nav_point_tag"
		") VALUES (?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for patrol_path: %s\n", sqlite3_errmsg(db));
		return;
	}

	for (int i = 0; i < factory->PatrolPath.Num(); i++) {
		ANavigationPoint* np = factory->PatrolPath.Data[i];
		if (!np) continue;
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
		sqlite3_bind_text  (stmt, 1, mapName.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int   (stmt, 2, mapObjectId);
		sqlite3_bind_int   (stmt, 3, i);
		sqlite3_bind_double(stmt, 4, np->Location.X);
		sqlite3_bind_double(stmt, 5, np->Location.Y);
		sqlite3_bind_double(stmt, 6, np->Location.Z);
		sqlite3_bind_text  (stmt, 7, FNameStr(np->Tag), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			Logger::Log("mapdump",
				"patrol_path insert failed factory=%d idx=%d: %s\n",
				mapObjectId, i, sqlite3_errmsg(db));
		}
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
