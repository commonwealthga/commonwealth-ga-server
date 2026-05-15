#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Bot.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgMissionObjective_Bot(sqlite3* db, AActor* actor,
                                 const std::string& mapName,
                                 const std::string& className,
                                 int mapObjectId) {
	auto* a = static_cast<ATgMissionObjective_Bot*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_mission_objective_bot ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" s_n_bot_id, s_n_spawn_table_id, s_b_auto_spawn, b_patrol_loop,"
		" f_balance, n_global_alarm_id"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_mission_objective_bot: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int   (stmt, 12, a->s_nBotId);
	sqlite3_bind_int   (stmt, 13, a->s_nSpawnTableId);
	sqlite3_bind_int   (stmt, 14, a->s_bAutoSpawn ? 1 : 0);
	sqlite3_bind_int   (stmt, 15, a->bPatrolLoop ? 1 : 0);
	sqlite3_bind_double(stmt, 16, a->fBalance);
	sqlite3_bind_int   (stmt, 17, a->nGlobalAlarmId);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_mission_objective_bot: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
