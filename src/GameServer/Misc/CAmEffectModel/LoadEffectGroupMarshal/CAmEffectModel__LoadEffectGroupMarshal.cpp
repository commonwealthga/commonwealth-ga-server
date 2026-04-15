#include "src/GameServer/Misc/CAmEffectModel/LoadEffectGroupMarshal/CAmEffectModel__LoadEffectGroupMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmEffectModel__LoadEffectGroupMarshal::bPopulateDatabaseEffectGroups = false;

void __fastcall CAmEffectModel__LoadEffectGroupMarshal::Call(void* CAmEffectGroupModelRow, void* edx, void* Marshal) {

	CallOriginal(CAmEffectGroupModelRow, edx, Marshal);

	// DB population moved to AsmDataCapture::WalkEffectGroups.
#if 0
	if (bPopulateDatabaseEffectGroups) {

		// CMarshal__Translate::m_values[GA_T::GA_T::NAME_MSG_ID]
		// CMarshal__Translate::m_values[GA_T::GA_T::DESC_MSG_ID]
		// CMarshal__GetFloat::m_values[GA_T::GA_T::LIFETIME_SEC]
		// CMarshal__GetFloat::m_values[GA_T::GA_T::APPLY_INTERVAL_SEC]
		// CMarshal__GetInt32t::m_values[GA_T::GA_T::TARGET_FX_ID]
		// CMarshal__GetInt32t::m_values[GA_T::GA_T::FX_DISPLAY_GROUP_RES_ID]
		// CMarshal__GetInt32t::m_values[GA_T::GA_T::ICON_ID]
		// CMarshal__GetByte::m_values[GA_T::GA_T::APPLICATION_VALUE_ID]
		// CMarshal__GetByte::m_values[GA_T::GA_T::CATEGORY_VALUE_ID]
		// CMarshal__GetFloat::m_values[GA_T::GA_T::APPLICATION_VALUE]
		// CMarshal__GetFloat::m_values[GA_T::GA_T::APPLICATION_CHANCE]
		// CMarshal__GetByte::m_values[GA_T::GA_T::SITUATIONAL_TYPE_VALUE_ID]
		// CMarshal__GetByte::m_values[GA_T::GA_T::REQUIRED_CATEGORY_VALUE_ID]
		// CMarshal__GetByte::m_values[GA_T::GA_T::REQUIRED_SKILL_ID]
		// CMarshal__GetByte::m_values[GA_T::GA_T::EFFECT_GROUP_TYPE_VALUE_ID]
		// CMarshal__GetByte::m_values[GA_T::GA_T::HEALTH]
		// CMarshal__GetFloat::m_values[GA_T::GA_T::SITUATIONAL_VALUE]
		// CMarshal__GetByte::m_values[GA_T::GA_T::STACK_COUNT_MAX]
		// CMarshal__GetByte::m_values[GA_T::GA_T::BUFF_VALUE]
		// CMarshal__GetFlag::m_values[GA_T::GA_T::CONTAGION_FLAG]
		// CMarshal__GetFlag::m_values[GA_T::GA_T::DEVICE_SPECIFIC_FLAG]
		// CMarshal__GetByte::m_values[GA_T::GA_T::POSTURE_TYPE_VALUE_ID]
		// CMarshal__GetInt32t::m_values[GA_T::GA_T::EFFECT_GROUP_ID]


		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		int result = sqlite3_prepare_v2(db,
			"INSERT INTO asm_data_set_effect_groups ( \
				effect_group_id, \
				lifetime_sec, \
				apply_interval_sec, \
				target_fx_id, \
				fx_display_group_res_id, \
				icon_id, \
				application_value_id, \
				category_value_id, \
				application_value, \
				application_chance, \
				situational_type_value_id, \
				required_category_value_id, \
				required_skill_id, \
				effect_group_type_value_id, \
				health, \
				situational_value, \
				stack_count_max, \
				buff_value, \
				contagion_flag, \
				device_specific_flag, \
				posture_type_value_id \
			) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, 0);

		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return;
		}

		sqlite3_bind_int  (stmt,  1, CMarshal__GetInt32t::m_values[GA_T::GA_T::EFFECT_GROUP_ID]);
		sqlite3_bind_double(stmt, 2, CMarshal__GetFloat::m_values[GA_T::GA_T::LIFETIME_SEC]);
		sqlite3_bind_double(stmt, 3, CMarshal__GetFloat::m_values[GA_T::GA_T::APPLY_INTERVAL_SEC]);
		sqlite3_bind_int  (stmt,  4, CMarshal__GetInt32t::m_values[GA_T::GA_T::TARGET_FX_ID]);
		sqlite3_bind_int  (stmt,  5, CMarshal__GetInt32t::m_values[GA_T::GA_T::FX_DISPLAY_GROUP_RES_ID]);
		sqlite3_bind_int  (stmt,  6, CMarshal__GetInt32t::m_values[GA_T::GA_T::ICON_ID]);
		sqlite3_bind_int  (stmt,  7, CMarshal__GetByte::m_values[GA_T::GA_T::APPLICATION_VALUE_ID]);
		sqlite3_bind_int  (stmt,  8, CMarshal__GetByte::m_values[GA_T::GA_T::CATEGORY_VALUE_ID]);
		sqlite3_bind_double(stmt, 9, CMarshal__GetFloat::m_values[GA_T::GA_T::APPLICATION_VALUE]);
		sqlite3_bind_double(stmt, 10, CMarshal__GetFloat::m_values[GA_T::GA_T::APPLICATION_CHANCE]);
		sqlite3_bind_int  (stmt, 11, CMarshal__GetByte::m_values[GA_T::GA_T::SITUATIONAL_TYPE_VALUE_ID]);
		sqlite3_bind_int  (stmt, 12, CMarshal__GetByte::m_values[GA_T::GA_T::REQUIRED_CATEGORY_VALUE_ID]);
		sqlite3_bind_int  (stmt, 13, CMarshal__GetByte::m_values[GA_T::GA_T::REQUIRED_SKILL_ID]);
		sqlite3_bind_int  (stmt, 14, CMarshal__GetByte::m_values[GA_T::GA_T::EFFECT_GROUP_TYPE_VALUE_ID]);
		sqlite3_bind_int  (stmt, 15, CMarshal__GetByte::m_values[GA_T::GA_T::HEALTH]);
		sqlite3_bind_double(stmt, 16, CMarshal__GetFloat::m_values[GA_T::GA_T::SITUATIONAL_VALUE]);
		sqlite3_bind_int  (stmt, 17, CMarshal__GetByte::m_values[GA_T::GA_T::STACK_COUNT_MAX]);
		sqlite3_bind_int  (stmt, 18, CMarshal__GetByte::m_values[GA_T::GA_T::BUFF_VALUE]);
		sqlite3_bind_int  (stmt, 19, CMarshal__GetFlag::m_values[GA_T::GA_T::CONTAGION_FLAG]);
		sqlite3_bind_int  (stmt, 20, CMarshal__GetFlag::m_values[GA_T::GA_T::DEVICE_SPECIFIC_FLAG]);
		sqlite3_bind_int  (stmt, 21, CMarshal__GetByte::m_values[GA_T::GA_T::POSTURE_TYPE_VALUE_ID]);

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
#endif

}


