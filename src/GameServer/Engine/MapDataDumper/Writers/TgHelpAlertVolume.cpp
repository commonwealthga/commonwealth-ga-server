#include "src/GameServer/Engine/MapDataDumper/Writers/TgHelpAlertVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgHelpAlertVolume(sqlite3* db, AActor* actor,
                            const std::string& mapName,
                            const std::string& className,
                            int mapObjectId) {
	auto* a = static_cast<ATgHelpAlertVolume*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_help_alert_volume ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_n_help_id"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_help_alert_volume: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->m_nHelpId);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_help_alert_volume: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
