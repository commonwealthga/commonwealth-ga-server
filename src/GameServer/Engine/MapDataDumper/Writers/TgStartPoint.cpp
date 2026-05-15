#include "src/GameServer/Engine/MapDataDumper/Writers/TgStartPoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgStartPoint(sqlite3* db, AActor* actor,
                       const std::string& mapName,
                       const std::string& className,
                       int mapObjectId) {
	auto* a = static_cast<ATgStartPoint*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_start_point ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_n_start_group, m_n_return_map_type, m_n_return_map_game_id,"
		" m_f_start_rating, m_f_current_rating, m_f_reset_rating, m_f_decrease_rate,"
		" availability_quest_group_id"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_start_point: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int   (stmt, 12, a->m_nStartGroup);
	sqlite3_bind_int   (stmt, 13, a->m_nReturnMapType);
	sqlite3_bind_int   (stmt, 14, a->m_nReturnMapGameId);
	sqlite3_bind_double(stmt, 15, a->m_fStartRating);
	sqlite3_bind_double(stmt, 16, a->m_fCurrentRating);
	sqlite3_bind_double(stmt, 17, a->m_fResetRating);
	sqlite3_bind_double(stmt, 18, a->m_fDecreaseRate);
	sqlite3_bind_int   (stmt, 19, a->AvailabilityQuestGroupId);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_start_point: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
