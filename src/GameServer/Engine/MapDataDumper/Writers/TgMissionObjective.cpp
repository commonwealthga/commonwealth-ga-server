#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace MapDumpWriters {

void WriteTgMissionObjective(sqlite3* db, AActor* actor,
                             const std::string& mapName,
                             const std::string& className,
                             int mapObjectId) {
	auto* a = static_cast<ATgMissionObjective*>(actor);

	const char* kSql =
		"INSERT INTO map_tg_mission_objective ("
		"map_name, map_object_id, class_name, tag, \"group\","
		" location_x, location_y, location_z,"
		" rotation_pitch, rotation_yaw, rotation_roll,"
		" b_has_no_location, r_b_use_pending_state, s_b_change_coalition_when_captured,"
		" m_b_capture_only_once, r_b_has_been_captured_once,"
		" s_b_end_overtime_on_defender_progress, b_enabled, r_b_is_locked,"
		" r_b_is_active, r_b_is_pending, m_b_pause_on_capture,"
		" c_b_local_pawn_is_attacker, m_b_block_advance, m_b_is_base_objective,"
		" c_b_is_local_player_attacker, c_b_local_player_attacker_cached,"
		" m_b_in_matinee_update, m_b_start_objective, s_b_random_picked,"
		" m_b_teleport_bots, n_priority, n_message_id, n_default_owner_task_force,"
		" n_objective_id, hex_bonus_direction, r_open_world_player_default_role,"
		" r_e_default_coalition, r_e_status, r_e_owning_coalition,"
		" s_open_world_bot_task_force, m_f_proximity_radius, m_f_proximity_height,"
		" m_f_time_to_capture, m_f_time_to_hold, m_n_points_per_second,"
		" m_n_type_id, m_n_desc_msg_id, m_n_icon_id, m_n_min_agents,"
		" m_n_cooldown_seconds, m_n_points, m_n_credits, m_n_reward_xp,"
		" m_n_loot_table_id, m_f_curr_capture_time, m_n_curr_owner_taskforce,"
		" r_n_owner_task_force, r_f_curr_capture_time, m_n_capture_time_ext_secs,"
		" m_n_capture_time_reset_secs, s_f_attacker_captured_at,"
		" r_f_last_completed_time, c_f_percentage, s_f_previous_play_rate,"
		" m_f_start_time, m_f_stop_time, s_n_previous_priority,"
		" s_n_current_spawn_order"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"  // common 11
		" ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"  // 20 bools
		" ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"  // 10 ints incl enums
		" ?, ?, ?, ?, ?,"  // 5 mixed
		" ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("mapdump", "prepare failed for map_tg_mission_objective: %s\n", sqlite3_errmsg(db));
		return;
	}
	BindCommonCols(stmt, actor, mapObjectId, mapName, className);
	sqlite3_bind_int(stmt, 12, a->bHasNoLocation ? 1 : 0);
	sqlite3_bind_int(stmt, 13, a->r_bUsePendingState ? 1 : 0);
	sqlite3_bind_int(stmt, 14, a->s_bChangeCoalitionWhenCaptured ? 1 : 0);
	sqlite3_bind_int(stmt, 15, a->m_bCaptureOnlyOnce ? 1 : 0);
	sqlite3_bind_int(stmt, 16, a->r_bHasBeenCapturedOnce ? 1 : 0);
	sqlite3_bind_int(stmt, 17, a->s_bEndOvertimeOnDefenderProgress ? 1 : 0);
	sqlite3_bind_int(stmt, 18, a->bEnabled ? 1 : 0);
	sqlite3_bind_int(stmt, 19, a->r_bIsLocked ? 1 : 0);
	sqlite3_bind_int(stmt, 20, a->r_bIsActive ? 1 : 0);
	sqlite3_bind_int(stmt, 21, a->r_bIsPending ? 1 : 0);
	sqlite3_bind_int(stmt, 22, a->m_bPauseOnCapture ? 1 : 0);
	sqlite3_bind_int(stmt, 23, a->c_bLocalPawnIsAttacker ? 1 : 0);
	sqlite3_bind_int(stmt, 24, a->m_bBlockAdvance ? 1 : 0);
	sqlite3_bind_int(stmt, 25, a->m_bIsBaseObjective ? 1 : 0);
	sqlite3_bind_int(stmt, 26, a->c_bIsLocalPlayerAttacker ? 1 : 0);
	sqlite3_bind_int(stmt, 27, a->c_bLocalPlayerAttackerCached ? 1 : 0);
	sqlite3_bind_int(stmt, 28, a->m_bInMatineeUpdate ? 1 : 0);
	sqlite3_bind_int(stmt, 29, a->m_bStartObjective ? 1 : 0);
	sqlite3_bind_int(stmt, 30, a->s_bRandomPicked ? 1 : 0);
	sqlite3_bind_int(stmt, 31, a->m_bTeleportBots ? 1 : 0);
	sqlite3_bind_int(stmt, 32, a->nPriority);
	sqlite3_bind_int(stmt, 33, a->nMessageId);
	sqlite3_bind_int(stmt, 34, a->nDefaultOwnerTaskForce);
	sqlite3_bind_int(stmt, 35, a->nObjectiveId);
	sqlite3_bind_int(stmt, 36, (int)a->HexBonusDirection);
	sqlite3_bind_int(stmt, 37, (int)a->r_OpenWorldPlayerDefaultRole);
	sqlite3_bind_int(stmt, 38, (int)a->r_eDefaultCoalition);
	sqlite3_bind_int(stmt, 39, (int)a->r_eStatus);
	sqlite3_bind_int(stmt, 40, (int)a->r_eOwningCoalition);
	sqlite3_bind_int(stmt, 41, a->s_OpenWorldBotTaskForce);
	sqlite3_bind_int   (stmt, 42, a->m_fProximityRadius);  // UC: var int despite m_f_ prefix
	sqlite3_bind_double(stmt, 43, a->m_fProximityHeight);
	sqlite3_bind_double(stmt, 44, a->m_fTimeToCapture);
	sqlite3_bind_double(stmt, 45, a->m_fTimeToHold);
	sqlite3_bind_int   (stmt, 46, a->m_nPointsPerSecond);
	sqlite3_bind_int   (stmt, 47, a->m_nTypeId);
	sqlite3_bind_int   (stmt, 48, a->m_nDescMsgId);
	sqlite3_bind_int   (stmt, 49, a->m_nIconId);
	sqlite3_bind_int   (stmt, 50, a->m_nMinAgents);
	sqlite3_bind_int   (stmt, 51, a->m_nCooldownSeconds);
	sqlite3_bind_int   (stmt, 52, a->m_nPoints);
	sqlite3_bind_int   (stmt, 53, a->m_nCredits);
	sqlite3_bind_int   (stmt, 54, a->m_nRewardXP);
	sqlite3_bind_int   (stmt, 55, a->m_nLootTableId);
	sqlite3_bind_double(stmt, 56, a->m_fCurrCaptureTime);
	sqlite3_bind_int   (stmt, 57, a->m_nCurrOwnerTaskforce);
	sqlite3_bind_int   (stmt, 58, a->r_nOwnerTaskForce);
	sqlite3_bind_double(stmt, 59, a->r_fCurrCaptureTime);
	sqlite3_bind_int   (stmt, 60, a->m_nCaptureTimeExtSecs);
	sqlite3_bind_int   (stmt, 61, a->m_nCaptureTimeResetSecs);
	sqlite3_bind_double(stmt, 62, a->s_fAttackerCapturedAt);
	sqlite3_bind_double(stmt, 63, a->r_fLastCompletedTime);
	sqlite3_bind_double(stmt, 64, a->c_fPercentage);
	sqlite3_bind_double(stmt, 65, a->s_fPreviousPlayRate);
	sqlite3_bind_double(stmt, 66, a->m_fStartTime);
	sqlite3_bind_double(stmt, 67, a->m_fStopTime);
	sqlite3_bind_int   (stmt, 68, a->s_nPreviousPriority);
	sqlite3_bind_int   (stmt, 69, a->s_nCurrentSpawnOrder);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("mapdump", "insert failed for map_tg_mission_objective: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
}

}  // namespace MapDumpWriters
