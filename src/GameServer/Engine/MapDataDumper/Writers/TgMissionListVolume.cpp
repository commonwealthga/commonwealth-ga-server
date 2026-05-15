#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionListVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgMissionListVolume(sqlite3* db, AActor* actor,
                              const std::string& mapName,
                              const std::string& className,
                              int mapObjectId) {
	auto* a = static_cast<ATgMissionListVolume*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_mission_list_volume ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" s_n_queue_table_id, s_n_queue_table_msg_id"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_mission_list_volume: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->s_nQueueTableId);
	sqlite3_bind_int(stmt, 13, a->s_nQueueTableMsgId);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_mission_list_volume: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
