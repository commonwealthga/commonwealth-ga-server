#include "src/GameServer/Engine/MapDataDumper/Writers/TgModifyPawnPropertiesVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgModifyPawnPropertiesVolume(sqlite3* db, AActor* actor,
                                       const std::string& mapName,
                                       const std::string& className,
                                       int mapObjectId) {
	auto* a = static_cast<ATgModifyPawnPropertiesVolume*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_modify_pawn_properties_volume ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_b_disable_jump, m_b_disable_block_actors, m_b_disable_hanging,"
		" m_b_disable_all_devices, m_b_trigger_use_event, m_b_one_way_movement,"
		" m_v_onew_way_pitch, m_v_onew_way_yaw, m_v_onew_way_roll, s_n_loot_table_id"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_modify_pawn_properties_volume: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->m_bDisableJump ? 1 : 0);
	sqlite3_bind_int(stmt, 13, a->m_bDisableBlockActors ? 1 : 0);
	sqlite3_bind_int(stmt, 14, a->m_bDisableHanging ? 1 : 0);
	sqlite3_bind_int(stmt, 15, a->m_bDisableAllDevices ? 1 : 0);
	sqlite3_bind_int(stmt, 16, a->m_bTriggerUseEvent ? 1 : 0);
	sqlite3_bind_int(stmt, 17, a->m_bOneWayMovement ? 1 : 0);
	sqlite3_bind_int(stmt, 18, a->m_vOnewWay.Pitch);
	sqlite3_bind_int(stmt, 19, a->m_vOnewWay.Yaw);
	sqlite3_bind_int(stmt, 20, a->m_vOnewWay.Roll);
	sqlite3_bind_int(stmt, 21, a->s_nLootTableId);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_modify_pawn_properties_volume: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
