#include "src/GameServer/Engine/MapDataDumper/Writers/TgTeamPlayerStart.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgTeamPlayerStart(sqlite3* db, AActor* actor,
                            const std::string& mapName,
                            const std::string& className,
                            int mapObjectId) {
	auto* a = static_cast<ATgTeamPlayerStart*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_team_player_start ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_n_task_force, m_e_coalition, m_n_priority, n_prev_priority, m_n_min_level"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_team_player_start: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, (int)a->m_nTaskForce);
	sqlite3_bind_int(stmt, 13, (int)a->m_eCoalition);
	sqlite3_bind_int(stmt, 14, a->m_nPriority);
	sqlite3_bind_int(stmt, 15, a->nPrevPriority);
	sqlite3_bind_int(stmt, 16, a->m_nMinLevel);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_team_player_start: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
