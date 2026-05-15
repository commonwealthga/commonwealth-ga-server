#include "src/GameServer/Engine/MapDataDumper/Writers/TgDeviceVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgDeviceVolume(sqlite3* db, AActor* actor,
                         const std::string& mapName,
                         const std::string& className,
                         int mapObjectId) {
	auto* a = static_cast<ATgDeviceVolume*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_device_volume ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" b_pain_causing, s_b_device_active, s_n_device_id, s_n_team_number, s_n_task_force"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_device_volume: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->bPainCausing ? 1 : 0);
	sqlite3_bind_int(stmt, 13, a->s_bDeviceActive ? 1 : 0);
	sqlite3_bind_int(stmt, 14, a->s_nDeviceId);
	sqlite3_bind_int(stmt, 15, a->s_nTeamNumber);
	sqlite3_bind_int(stmt, 16, (int)a->s_nTaskForce);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_device_volume: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
