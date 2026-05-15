#include "src/GameServer/Engine/MapDataDumper/Writers/TgBotFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryLocationList.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBotFactoryPatrolPath.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgBotFactory(sqlite3* db, AActor* actor,
                       const std::string& mapName,
                       const std::string& className,
                       int mapObjectId) {
	auto* a = static_cast<ATgBotFactory*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_bot_factory ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" location_selection, type_selection, s_n_cur_location_index,"
		" b_patrol_loop, b_always_patrol, b_spawn_on_alarm, b_auto_spawn,"
		" m_b_first_spawn, b_bulk_spawn, b_respawn, m_b_ignore_collision_on_spawn,"
		" n_global_alarm_id, n_bot_count, n_current_count, n_active_count,"
		" n_total_spawns, n_spawn_table_id, n_default_spawn_table_id,"
		" f_spawn_delay, m_n_last_group, n_priority, n_prev_priority,"
		" f_last_kill_time, f_balance, f_respawn_delay, m_n_spawn_order"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
		" ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_bot_factory: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int   (stmt, 12, (int)a->LocationSelection);
	sqlite3_bind_int   (stmt, 13, (int)a->TypeSelection);
	sqlite3_bind_int   (stmt, 14, a->s_nCurLocationIndex);
	sqlite3_bind_int   (stmt, 15, a->bPatrolLoop ? 1 : 0);
	sqlite3_bind_int   (stmt, 16, a->bAlwaysPatrol ? 1 : 0);
	sqlite3_bind_int   (stmt, 17, a->bSpawnOnAlarm ? 1 : 0);
	sqlite3_bind_int   (stmt, 18, a->bAutoSpawn ? 1 : 0);
	sqlite3_bind_int   (stmt, 19, a->m_bFirstSpawn ? 1 : 0);
	sqlite3_bind_int   (stmt, 20, a->bBulkSpawn ? 1 : 0);
	sqlite3_bind_int   (stmt, 21, a->bRespawn ? 1 : 0);
	sqlite3_bind_int   (stmt, 22, a->m_bIgnoreCollisionOnSpawn ? 1 : 0);
	sqlite3_bind_int   (stmt, 23, a->nGlobalAlarmId);
	sqlite3_bind_int   (stmt, 24, a->nBotCount);
	sqlite3_bind_int   (stmt, 25, a->nCurrentCount);
	sqlite3_bind_int   (stmt, 26, a->nActiveCount);
	sqlite3_bind_int   (stmt, 27, a->nTotalSpawns);
	sqlite3_bind_int   (stmt, 28, a->nSpawnTableId);
	sqlite3_bind_int   (stmt, 29, a->nDefaultSpawnTableId);
	sqlite3_bind_double(stmt, 30, a->fSpawnDelay);
	sqlite3_bind_int   (stmt, 31, a->m_nLastGroup);
	sqlite3_bind_int   (stmt, 32, a->nPriority);
	sqlite3_bind_int   (stmt, 33, a->nPrevPriority);
	sqlite3_bind_double(stmt, 34, a->fLastKillTime);
	sqlite3_bind_double(stmt, 35, a->fBalance);
	sqlite3_bind_double(stmt, 36, a->fRespawnDelay);
	sqlite3_bind_int   (stmt, 37, a->m_nSpawnOrder);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_bot_factory: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);

	// Fan out into the array tables once we've recorded the factory itself.
	WriteTgBotFactoryLocationList(db, a, mapName, mapObjectId);
	WriteTgBotFactoryPatrolPath  (db, a, mapName, mapObjectId);
}

}  // namespace MapDumpWriters
