#include "src/GameServer/Engine/MapDataDumper/Writers/TgPointOfInterest.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgPointOfInterest(sqlite3* db, AActor* actor,
                            const std::string& mapName,
                            const std::string& className,
                            int mapObjectId) {
	auto* a = static_cast<ATgPointOfInterest*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_point_of_interest ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_n_name_msg_id, m_s_debug_name, m_quest_radius_uu, m_b_show_when_quest_complete"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_point_of_interest: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->m_nNameMsgId);
	std::string debugName = FStringToUtf8(a->m_sDebugName);
	sqlite3_bind_text  (stmt, 13, debugName.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt, 14, a->m_QuestRadiusUU);
	sqlite3_bind_int   (stmt, 15, a->m_bShowWhenQuestComplete ? 1 : 0);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_point_of_interest: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
