#include "src/GameServer/Engine/MapDataDumper/Writers/TgTeleporter.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgTeleporter(sqlite3* db, AActor* actor,
                       const std::string& mapName,
                       const std::string& className,
                       int mapObjectId) {
	auto* a = static_cast<ATgTeleporter*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_teleporter ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_n_map_id, m_n_preload, m_b_set_task_force, m_b_balance_task_force,"
		" m_b_ignore_non_members, m_b_use_player_start, m_b_request_mission,"
		" m_n_start_group, m_n_task_force"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_teleporter: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->m_nMapId);
	sqlite3_bind_int(stmt, 13, a->m_nPreload ? 1 : 0);
	sqlite3_bind_int(stmt, 14, a->m_bSetTaskForce ? 1 : 0);
	sqlite3_bind_int(stmt, 15, a->m_bBalanceTaskForce ? 1 : 0);
	sqlite3_bind_int(stmt, 16, a->m_bIgnoreNonMembers ? 1 : 0);
	sqlite3_bind_int(stmt, 17, a->m_bUsePlayerStart ? 1 : 0);
	sqlite3_bind_int(stmt, 18, a->m_bRequestMission ? 1 : 0);
	sqlite3_bind_int(stmt, 19, (int)a->m_nStartGroup);
	sqlite3_bind_int(stmt, 20, (int)a->m_nTaskForce);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_teleporter: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
