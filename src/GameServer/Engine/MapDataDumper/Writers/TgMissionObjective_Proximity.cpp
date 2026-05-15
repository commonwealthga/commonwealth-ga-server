#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Proximity.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgMissionObjective_Proximity(sqlite3* db, AActor* actor,
                                       const std::string& mapName,
                                       const std::string& className,
                                       int mapObjectId) {
	auto* a = static_cast<ATgMissionObjective_Proximity*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_mission_objective_proximity ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" s_b_allow_ai_bot_interaction, s_b_allow_remote_control_bot_interaction,"
		" m_f_def_capture_rate, m_f_capture_accel_rate, m_f_capture_accel_rate_cap,"
		" r_f_capture_rate, m_f_overtime_accel_rate"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_mission_objective_proximity: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int   (stmt, 12, a->s_bAllowAIBotInteraction ? 1 : 0);
	sqlite3_bind_int   (stmt, 13, a->s_bAllowRemoteControlBotInteraction ? 1 : 0);
	sqlite3_bind_double(stmt, 14, a->m_fDefCaptureRate);
	sqlite3_bind_double(stmt, 15, a->m_fCaptureAccelRate);
	sqlite3_bind_double(stmt, 16, a->m_fCaptureAccelRateCap);
	sqlite3_bind_double(stmt, 17, a->r_fCaptureRate);
	sqlite3_bind_double(stmt, 18, a->m_fOvertimeAccelRate);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_mission_objective_proximity: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
