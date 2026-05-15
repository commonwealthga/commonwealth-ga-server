#include "src/GameServer/Engine/MapDataDumper/Writers/TgCoverPoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgCoverPoint(sqlite3* db, AActor* actor,
                       const std::string& mapName,
                       const std::string& className,
                       int mapObjectId) {
	auto* a = static_cast<ATgCoverPoint*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_cover_point ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_b_lean_left, m_b_lean_right, m_b_allow_popup, m_b_allow_mantle,"
		" m_v_lean_left_x, m_v_lean_left_y, m_v_lean_left_z,"
		" m_v_lean_right_x, m_v_lean_right_y, m_v_lean_right_z,"
		" m_v_pop_up_x, m_v_pop_up_y, m_v_pop_up_z"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
		" ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_cover_point: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int   (stmt, 12, a->m_bLeanLeft ? 1 : 0);
	sqlite3_bind_int   (stmt, 13, a->m_bLeanRight ? 1 : 0);
	sqlite3_bind_int   (stmt, 14, a->m_bAllowPopup ? 1 : 0);
	sqlite3_bind_int   (stmt, 15, a->m_bAllowMantle ? 1 : 0);
	sqlite3_bind_double(stmt, 16, a->m_vLeanLeft.X);
	sqlite3_bind_double(stmt, 17, a->m_vLeanLeft.Y);
	sqlite3_bind_double(stmt, 18, a->m_vLeanLeft.Z);
	sqlite3_bind_double(stmt, 19, a->m_vLeanRight.X);
	sqlite3_bind_double(stmt, 20, a->m_vLeanRight.Y);
	sqlite3_bind_double(stmt, 21, a->m_vLeanRight.Z);
	sqlite3_bind_double(stmt, 22, a->m_vPopUp.X);
	sqlite3_bind_double(stmt, 23, a->m_vPopUp.Y);
	sqlite3_bind_double(stmt, 24, a->m_vPopUp.Z);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_cover_point: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
