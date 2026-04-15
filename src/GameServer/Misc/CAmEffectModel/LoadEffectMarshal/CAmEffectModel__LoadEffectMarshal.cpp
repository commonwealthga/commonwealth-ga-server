#include "src/GameServer/Misc/CAmEffectModel/LoadEffectMarshal/CAmEffectModel__LoadEffectMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/GetFlag/CMarshal__GetFlag.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmEffectModel__LoadEffectMarshal::bPopulateDatabaseEffects = false;

void __fastcall CAmEffectModel__LoadEffectMarshal::Call(void* CAmEffectModelRow, void* edx, void* Marshal) {

	CallOriginal(CAmEffectModelRow, edx, Marshal);

	// DB population moved to AsmDataCapture::WalkEffects.
#if 0
	if (bPopulateDatabaseEffects) {

		// CMarshal__GetInt32t::m_values[GA_T::GA_T::EFFECT_GROUP_ID]
		// CMarshal__GetInt32t::m_values[GA_T::GA_T::EFFECT_ID]
		// CMarshal__GetInt32t::m_values[GA_T::GA_T::CLASS_RES_ID]
		// CMarshal__GetFloat::m_values[GA_T::GA_T::BASE_VALUE]
		// CMarshal__GetFloat::m_values[GA_T::GA_T::MIN_VALUE]
		// CMarshal__GetFloat::m_values[GA_T::GA_T::MAX_VALUE]
		// CMarshal__GetInt32t::m_values[GA_T::GA_T::CALC_METHOD_VALUE_ID]
		// CMarshal__GetByte::m_values[GA_T::GA_T::PROP_ID]
		// CMarshal__GetInt32t::m_values[GA_T::GA_T::PROPERTY_VALUE_ID]
		// CMarshal__GetFlag::m_values[GA_T::GA_T::APPLY_ON_INTERVAL_FLAG]

		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		int result = sqlite3_prepare_v2(db,
			"INSERT INTO asm_data_set_effects ( \
				effect_group_id, \
				effect_id, \
				class_res_id, \
				base_value, \
				min_value, \
				max_value, \
				calc_method_value_id, \
				prop_id, \
				property_value_id, \
				apply_on_interval_flag \
			) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, 0);

		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return;
		}

		sqlite3_bind_int   (stmt,  1, CMarshal__GetInt32t::m_values[GA_T::GA_T::EFFECT_GROUP_ID]);
		sqlite3_bind_int   (stmt,  2, CMarshal__GetInt32t::m_values[GA_T::GA_T::EFFECT_ID]);
		sqlite3_bind_int   (stmt,  3, CMarshal__GetInt32t::m_values[GA_T::GA_T::CLASS_RES_ID]);
		sqlite3_bind_double(stmt,  4, CMarshal__GetFloat::m_values[GA_T::GA_T::BASE_VALUE]);
		sqlite3_bind_double(stmt,  5, CMarshal__GetFloat::m_values[GA_T::GA_T::MIN_VALUE]);
		sqlite3_bind_double(stmt,  6, CMarshal__GetFloat::m_values[GA_T::GA_T::MAX_VALUE]);
		sqlite3_bind_int   (stmt,  7, CMarshal__GetInt32t::m_values[GA_T::GA_T::CALC_METHOD_VALUE_ID]);
		sqlite3_bind_int   (stmt,  8, CMarshal__GetByte::m_values[GA_T::GA_T::PROP_ID]);
		sqlite3_bind_int   (stmt,  9, CMarshal__GetInt32t::m_values[GA_T::GA_T::PROPERTY_VALUE_ID]);
		sqlite3_bind_int   (stmt, 10, CMarshal__GetFlag::m_values[GA_T::GA_T::APPLY_ON_INTERVAL_FLAG]);

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
#endif

}


