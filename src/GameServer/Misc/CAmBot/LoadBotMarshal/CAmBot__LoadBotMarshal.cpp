#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmBot__LoadBotMarshal::bPopulateDatabaseBots = false;
bool CAmBot__LoadBotMarshal::bPopulateDatabaseBotDevices = false;
std::map<uint32_t, int> CAmBot__LoadBotMarshal::m_BotPointers;
std::map<uint32_t, uint32_t> CAmBot__LoadBotMarshal::m_BotBehaviorIds;

void __fastcall CAmBot__LoadBotMarshal::Call(void* CAmBotRow, void* edx, void* Marshal) {
	// bIsLoadingBots / m_botDevices side-channel is obsolete — bot device capture
	// now happens in AsmDataCapture::WalkBotDevices (driven by CMarshal__GetArray
	// dispatch for DATA_SET_BOT_DEVICES).
	CallOriginal(CAmBotRow, edx, Marshal);

	m_BotPointers[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]] = (int)(CAmBotRow);
	m_BotBehaviorIds[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]] = CMarshal__GetByte::m_values[GA_T::GA_T::BEHAVIOR_ID];

	// DB population moved to AsmDataCapture (bots + nested bot_devices).  Keeping
	// bPopulateDatabaseBots / bPopulateDatabaseBotDevices fields for source-compat;
	// they're read but never true.  Original INSERT code preserved in #if 0 block.
#if 0
	if (bPopulateDatabaseBots) {
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		int result = sqlite3_prepare_v2(db,
			"INSERT INTO asm_data_set_bots ( \
				bot_id, \
				reference_name, \
				name_msg_id, \
				desc_msg_id, \
				level, \
				pawn_class_res_id, \
				controller_class_res_id, \
				behavior_id, \
				head_asm_id, \
				body_asm_id, \
				movement_asm_id, \
				hit_points, \
				bot_type_value_id, \
				physical_type_value_id, \
				default_slot_value_id, \
				default_sensor_range, \
				default_aggro_range, \
				default_help_range, \
				hearing_range, \
				default_speed, \
				walk_speed_pct, \
				crouch_speed_pct, \
				chase_range, \
				chase_time_sec, \
				stealth_sensor_range, \
				stealth_aggro_range, \
				hibernate_on_idle_sec, \
				hibernate_delay_rate, \
				icon_id, \
				bot_rank_value_id, \
				target_only_physical_type_value_id, \
				skill_group_id, \
				skill_group_set_id, \
				fixed_fov_degrees, \
				loot_table_id, \
				default_power_pool, \
				rotation_rate, \
				class_type_value_id, \
				device_slot_unlock_group_id, \
				pickup_device_id, \
				xp_value, \
				currency_value, \
				squad_role_value_id, \
				default_posture_value_id, \
				acceleration_rate, \
				accuracy_override, \
				bot_balance_multiplier, \
				power_pool_regen_per_sec, \
				hibernate_invulnerability_flag, \
				can_jump_flag, \
				can_climb_ladders_flag, \
				path_only_flag, \
				always_load_on_server_flag, \
				destroy_on_owner_death_flag \
			) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", -1, &stmt, 0);

		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return;
		}

		sqlite3_bind_int(stmt,  1, CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]);
		sqlite3_bind_text(stmt, 2, CMarshal__GetString2::m_values[GA_T::GA_T::REFERENCE_NAME], -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt,  3, CMarshal__GetByte::m_values[GA_T::GA_T::NAME_MSG_ID]);
		sqlite3_bind_int(stmt,  4, CMarshal__GetByte::m_values[GA_T::GA_T::DESC_MSG_ID]);
		sqlite3_bind_int(stmt,  5, CMarshal__GetByte::m_values[GA_T::GA_T::LEVEL]);
		sqlite3_bind_int(stmt,  6, CMarshal__GetInt32t::m_values[GA_T::GA_T::PAWN_CLASS_RES_ID]);
		sqlite3_bind_int(stmt,  7, CMarshal__GetInt32t::m_values[GA_T::GA_T::CONTROLLER_CLASS_RES_ID]);
		sqlite3_bind_int(stmt,  8, CMarshal__GetByte::m_values[GA_T::GA_T::BEHAVIOR_ID]);
		sqlite3_bind_int(stmt,  9, CMarshal__GetByte::m_values[GA_T::GA_T::HEAD_ASM_ID]);
		sqlite3_bind_int(stmt, 10, CMarshal__GetByte::m_values[GA_T::GA_T::BODY_ASM_ID]);
		sqlite3_bind_int(stmt, 11, CMarshal__GetByte::m_values[GA_T::GA_T::MOVEMENT_ASM_ID]);
		sqlite3_bind_int(stmt, 12, CMarshal__GetByte::m_values[GA_T::GA_T::HIT_POINTS]);
		sqlite3_bind_int(stmt, 13, CMarshal__GetByte::m_values[GA_T::GA_T::BOT_TYPE_VALUE_ID]);
		sqlite3_bind_int(stmt, 14, CMarshal__GetByte::m_values[GA_T::GA_T::PHYSICAL_TYPE_VALUE_ID]);
		sqlite3_bind_int(stmt, 15, CMarshal__GetByte::m_values[GA_T::GA_T::DEFAULT_SLOT_VALUE_ID]);
		sqlite3_bind_double(stmt, 16, CMarshal__GetFloat::m_values[GA_T::GA_T::DEFAULT_SENSOR_RANGE]);
		sqlite3_bind_int(stmt,  17, CMarshal__GetByte::m_values[GA_T::GA_T::DEFAULT_AGGRO_RANGE]);
		sqlite3_bind_int(stmt,  18, CMarshal__GetByte::m_values[GA_T::GA_T::DEFAULT_HELP_RANGE]);
		sqlite3_bind_double(stmt, 19, CMarshal__GetFloat::m_values[GA_T::GA_T::HEARING_RANGE]);
		sqlite3_bind_double(stmt, 20, CMarshal__GetFloat::m_values[GA_T::GA_T::DEFAULT_SPEED]);
		sqlite3_bind_double(stmt, 21, CMarshal__GetFloat::m_values[GA_T::GA_T::WALK_SPEED_PCT]);
		sqlite3_bind_double(stmt, 22, CMarshal__GetFloat::m_values[GA_T::GA_T::CROUCH_SPEED_PCT]);
		sqlite3_bind_int(stmt,  23, CMarshal__GetByte::m_values[GA_T::GA_T::CHASE_RANGE]);
		sqlite3_bind_double(stmt, 24, CMarshal__GetFloat::m_values[GA_T::GA_T::CHASE_TIME_SEC]);
		sqlite3_bind_int(stmt,  25, CMarshal__GetByte::m_values[GA_T::GA_T::STEALTH_SENSOR_RANGE]);
		sqlite3_bind_int(stmt,  26, CMarshal__GetByte::m_values[GA_T::GA_T::STEALTH_AGGRO_RANGE]);
		sqlite3_bind_int(stmt,  27, CMarshal__GetByte::m_values[GA_T::GA_T::HIBERNATE_ON_IDLE_SEC]);
		sqlite3_bind_double(stmt, 28, CMarshal__GetFloat::m_values[GA_T::GA_T::HIBERNATE_DELAY_RATE]);
		sqlite3_bind_int(stmt,  29, CMarshal__GetByte::m_values[GA_T::GA_T::ICON_ID]);
		sqlite3_bind_int(stmt,  30, CMarshal__GetByte::m_values[GA_T::GA_T::BOT_RANK_VALUE_ID]);
		sqlite3_bind_int(stmt,  31, CMarshal__GetByte::m_values[GA_T::GA_T::TARGET_ONLY_PHYSICAL_TYPE_VALUE_ID]);
		sqlite3_bind_int(stmt,  32, CMarshal__GetByte::m_values[GA_T::GA_T::SKILL_GROUP_ID]);
		sqlite3_bind_int(stmt,  33, CMarshal__GetByte::m_values[GA_T::GA_T::SKILL_GROUP_SET_ID]);
		sqlite3_bind_int(stmt,  34, CMarshal__GetByte::m_values[GA_T::GA_T::FIXED_FOV_DEGREES]);
		sqlite3_bind_int(stmt,  35, CMarshal__GetByte::m_values[GA_T::GA_T::LOOT_TABLE_ID]);
		sqlite3_bind_int(stmt,  36, CMarshal__GetByte::m_values[GA_T::GA_T::DEFAULT_POWER_POOL]);
		sqlite3_bind_int(stmt,  37, CMarshal__GetByte::m_values[GA_T::GA_T::ROTATION_RATE]);
		sqlite3_bind_int(stmt,  38, CMarshal__GetByte::m_values[GA_T::GA_T::CLASS_TYPE_VALUE_ID]);
		sqlite3_bind_int(stmt,  39, CMarshal__GetByte::m_values[GA_T::GA_T::DEVICE_SLOT_UNLOCK_GROUP_ID]);
		sqlite3_bind_int(stmt,  40, CMarshal__GetByte::m_values[GA_T::GA_T::PICKUP_DEVICE_ID]);
		sqlite3_bind_int(stmt,  41, CMarshal__GetByte::m_values[GA_T::GA_T::XP_VALUE]);
		sqlite3_bind_int(stmt,  42, CMarshal__GetByte::m_values[GA_T::GA_T::CURRENCY_VALUE]);
		sqlite3_bind_int(stmt,  43, CMarshal__GetByte::m_values[GA_T::GA_T::SQUAD_ROLE_VALUE_ID]);
		sqlite3_bind_int(stmt,  44, CMarshal__GetByte::m_values[GA_T::GA_T::DEFAULT_POSTURE_VALUE_ID]);
		sqlite3_bind_double(stmt, 45, CMarshal__GetFloat::m_values[GA_T::GA_T::ACCELERATION_RATE]);
		sqlite3_bind_double(stmt, 46, CMarshal__GetFloat::m_values[GA_T::GA_T::ACCURACY_OVERRIDE]);
		sqlite3_bind_double(stmt, 47, CMarshal__GetFloat::m_values[GA_T::GA_T::BOT_BALANCE_MULTIPLIER]);
		sqlite3_bind_double(stmt, 48, CMarshal__GetFloat::m_values[GA_T::GA_T::POWER_POOL_REGEN_PER_SEC]);
		sqlite3_bind_int(stmt,  49, CMarshal__GetFlag::m_values[GA_T::GA_T::HIBERNATE_INVULNERABILITY_FLAG]);
		sqlite3_bind_int(stmt,  50, CMarshal__GetFlag::m_values[GA_T::GA_T::CAN_JUMP_FLAG]);
		sqlite3_bind_int(stmt,  51, CMarshal__GetFlag::m_values[GA_T::GA_T::CAN_CLIMB_LADDERS_FLAG]);
		sqlite3_bind_int(stmt,  52, CMarshal__GetFlag::m_values[GA_T::GA_T::PATH_ONLY_FLAG]);
		sqlite3_bind_int(stmt,  53, CMarshal__GetFlag::m_values[GA_T::GA_T::ALWAYS_LOAD_ON_SERVER_FLAG]);
		sqlite3_bind_int(stmt,  54, CMarshal__GetFlag::m_values[GA_T::GA_T::DESTROY_ON_OWNER_DEATH_FLAG]);

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	if (bPopulateDatabaseBotDevices) {
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;

		if (CMarshal__GetByte::m_botDevices[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]].size() == 0) {
			return;
		}

		for (auto& device : CMarshal__GetByte::m_botDevices[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]]) {
			int result = sqlite3_prepare_v2(db,
				"INSERT INTO asm_data_set_bots_data_set_bot_devices ( \
					bot_id, \
					device_id, \
					slot_used_value_id \
				) VALUES (?, ?, ?)", -1, &stmt, 0);

			if (result != SQLITE_OK) {
				Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
				return;
			}

			sqlite3_bind_int(stmt, 1, CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]);
			sqlite3_bind_int(stmt, 2, device.DeviceId);
			sqlite3_bind_int(stmt, 3, device.SlotUsedValueId);

			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}
#endif
}

