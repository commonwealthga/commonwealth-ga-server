#include "src/GameServer/Engine/MapDataDumper/Writers/TgOmegaVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgOmegaVolume(sqlite3* db, AActor* actor,
                        const std::string& mapName,
                        const std::string& className,
                        int mapObjectId) {
	auto* a = static_cast<ATgOmegaVolume*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_omega_volume ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" m_n_omega_alert_id, m_n_subzone_name_msg_id, m_n_subzone_secondary_name_msg_id,"
		" m_n_omega_priority, m_n_bilboard_key, m_b_enable_equip, m_b_enable_skills,"
		" m_b_enable_crafting, m_b_auto_kick_if_idle, m_e_visual_cue"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_omega_volume: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->m_nOmegaAlertId);
	sqlite3_bind_int(stmt, 13, a->m_nSubzoneNameMsgId);
	sqlite3_bind_int(stmt, 14, a->m_nSubzoneSecondaryNameMsgId);
	sqlite3_bind_int(stmt, 15, a->m_nOmegaPriority);
	sqlite3_bind_int(stmt, 16, a->m_nBilboardKey);
	sqlite3_bind_int(stmt, 17, a->m_bEnableEquip ? 1 : 0);
	sqlite3_bind_int(stmt, 18, a->m_bEnableSkills ? 1 : 0);
	sqlite3_bind_int(stmt, 19, a->m_bEnableCrafting ? 1 : 0);
	sqlite3_bind_int(stmt, 20, a->m_bAutoKickIfIdle ? 1 : 0);
	sqlite3_bind_int(stmt, 21, (int)a->m_eVisualCue);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_omega_volume: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
