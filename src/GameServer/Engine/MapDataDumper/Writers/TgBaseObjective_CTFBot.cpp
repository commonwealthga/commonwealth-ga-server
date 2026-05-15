#include "src/GameServer/Engine/MapDataDumper/Writers/TgBaseObjective_CTFBot.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgBaseObjective_CTFBot(sqlite3* db, AActor* actor,
                                 const std::string& mapName,
                                 const std::string& className,
                                 int mapObjectId) {
	auto* a = static_cast<ATgBaseObjective_CTFBot*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_base_objective_ctf_bot ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_f_flag_respawn_delay, m_f_scoring_radius,"
		" m_b_spawn_unaligned_bot, m_b_capture_on_death"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_base_objective_ctf_bot: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_double(stmt, 12, a->m_fFlagRespawnDelay);
	sqlite3_bind_double(stmt, 13, a->m_fScoringRadius);
	sqlite3_bind_int   (stmt, 14, a->m_bSpawnUnalignedBot ? 1 : 0);
	sqlite3_bind_int   (stmt, 15, a->m_bCaptureOnDeath ? 1 : 0);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_base_objective_ctf_bot: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
