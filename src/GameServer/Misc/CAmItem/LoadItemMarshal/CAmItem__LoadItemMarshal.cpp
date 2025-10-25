#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmItem__LoadItemMarshal::bPopulateDatabaseItems = false;
std::map<uint32_t, int> CAmItem__LoadItemMarshal::m_ItemPointers;

void __fastcall CAmItem__LoadItemMarshal::Call(void* CAmItemRow, void* edx, void* Marshal) {

	CallOriginal(CAmItemRow, edx, Marshal);

	m_ItemPointers[CMarshal__GetByte::m_values[GA_T::GA_T::ITEM_ID]] = (int)(CAmItemRow);

	if (bPopulateDatabaseItems) {
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		int result = sqlite3_prepare_v2(db,
			"INSERT INTO asm_data_set_items ( \
				name_msg_id, \
				name_msg_translated, \
				desc_msg_id, \
				desc_msg_translated, \
				class_res_id, \
				item_id, \
				item_type_value_id, \
				item_subtype_value_id, \
				skill_id, \
				sub_skill_id, \
				skill_level_min, \
				quantity, \
				icon_id, \
				weight, \
				time_to_live_secs, \
				quality_value_id, \
				required_achievement_id, \
				required_achievement_points, \
				ref_bot_id, \
				ref_deployable_id, \
				ref_device_id, \
				item_bind_type_value_id, \
				production_cost, \
				required_level, \
				purchased_value, \
				bundle_loot_table_id \
			) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, 0);

		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return;
		}

		sqlite3_bind_int(stmt, 1, CMarshal__GetInt32t::m_values[GA_T::GA_T::NAME_MSG_ID]);
		sqlite3_bind_text(stmt, 2, CMarshal__Translate::m_values[GA_T::GA_T::NAME_MSG_ID], -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 3, CMarshal__GetInt32t::m_values[GA_T::GA_T::DESC_MSG_ID]);
		sqlite3_bind_text(stmt, 4, CMarshal__Translate::m_values[GA_T::GA_T::DESC_MSG_ID], -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 5, CMarshal__GetInt32t::m_values[GA_T::GA_T::CLASS_RES_ID]);
		sqlite3_bind_int(stmt, 6, CMarshal__GetByte::m_values[GA_T::GA_T::ITEM_ID]);
		sqlite3_bind_int(stmt, 7, CMarshal__GetByte::m_values[GA_T::GA_T::ITEM_TYPE_VALUE_ID]);
		sqlite3_bind_int(stmt, 8, CMarshal__GetByte::m_values[GA_T::GA_T::ITEM_SUBTYPE_VALUE_ID]);
		sqlite3_bind_int(stmt, 9, CMarshal__GetByte::m_values[GA_T::GA_T::SKILL_ID]);
		sqlite3_bind_int(stmt, 10, CMarshal__GetByte::m_values[GA_T::GA_T::SUB_SKILL_ID]);
		sqlite3_bind_int(stmt, 11, CMarshal__GetByte::m_values[GA_T::GA_T::SKILL_LEVEL_MIN]);
		sqlite3_bind_int(stmt, 12, CMarshal__GetByte::m_values[GA_T::GA_T::QUANTITY]);
		sqlite3_bind_int(stmt, 13, CMarshal__GetByte::m_values[GA_T::GA_T::ICON_ID]);
		sqlite3_bind_int(stmt, 14, CMarshal__GetFloat::m_values[GA_T::GA_T::WEIGHT]);
		sqlite3_bind_int(stmt, 15, CMarshal__GetFloat::m_values[GA_T::GA_T::TIME_TO_LIVE_SECS]);
		sqlite3_bind_int(stmt, 16, CMarshal__GetByte::m_values[GA_T::GA_T::QUALITY_VALUE_ID]);
		sqlite3_bind_int(stmt, 17, CMarshal__GetByte::m_values[GA_T::GA_T::REQUIRED_ACHIEVEMENT_ID]);
		sqlite3_bind_int(stmt, 18, CMarshal__GetByte::m_values[GA_T::GA_T::REQUIRED_ACHIEVEMENT_POINTS]);
		sqlite3_bind_int(stmt, 19, CMarshal__GetByte::m_values[GA_T::GA_T::REF_BOT_ID]);
		sqlite3_bind_int(stmt, 20, CMarshal__GetByte::m_values[GA_T::GA_T::REF_DEPLOYABLE_ID]);
		sqlite3_bind_int(stmt, 21, CMarshal__GetByte::m_values[GA_T::GA_T::REF_DEVICE_ID]);
		sqlite3_bind_int(stmt, 22, CMarshal__GetByte::m_values[GA_T::GA_T::ITEM_BIND_TYPE_VALUE_ID]);
		sqlite3_bind_int(stmt, 23, CMarshal__GetByte::m_values[GA_T::GA_T::PRODUCTION_COST]);
		sqlite3_bind_int(stmt, 24, CMarshal__GetByte::m_values[GA_T::GA_T::REQUIRED_LEVEL]);
		sqlite3_bind_int(stmt, 25, CMarshal__GetByte::m_values[GA_T::GA_T::PURCHASED_VALUE]);
		sqlite3_bind_int(stmt, 26, CMarshal__GetByte::m_values[GA_T::GA_T::BUNDLE_LOOT_TABLE_ID]);

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

