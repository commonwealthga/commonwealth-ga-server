#include "src/GameServer/Engine/MapDataDumper/Writers/TgActorFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgActorFactory(sqlite3* db, AActor* actor,
                         const std::string& mapName,
                         const std::string& className,
                         int mapObjectId) {
	auto* a = static_cast<ATgActorFactory*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_actor_factory ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" s_b_auto_spawn, s_n_team_number, s_n_task_force, s_e_coalition,"
		" s_e_selection_method, s_n_selection_list_id, s_n_selected_object_id,"
		" m_n_selection_list_prop_id, s_n_name_id, s_n_cur_list_index"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_actor_factory: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->s_bAutoSpawn ? 1 : 0);
	sqlite3_bind_int(stmt, 13, a->s_nTeamNumber);
	sqlite3_bind_int(stmt, 14, (int)a->s_nTaskForce);
	sqlite3_bind_int(stmt, 15, (int)a->s_eCoalition);
	sqlite3_bind_int(stmt, 16, (int)a->s_eSelectionMethod);
	sqlite3_bind_int(stmt, 17, a->s_nSelectionListId);
	sqlite3_bind_int(stmt, 18, a->s_nSelectedObjectId);
	sqlite3_bind_int(stmt, 19, a->m_nSelectionListPropId);
	sqlite3_bind_int(stmt, 20, a->s_nNameId);
	sqlite3_bind_int(stmt, 21, a->s_nCurListIndex);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_actor_factory: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
