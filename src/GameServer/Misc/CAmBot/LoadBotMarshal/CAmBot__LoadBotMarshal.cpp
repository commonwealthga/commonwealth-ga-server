#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmBot__LoadBotMarshal::bPopulateDatabase = false;
std::map<uint32_t, int> CAmBot__LoadBotMarshal::m_BotPointers;
std::map<uint32_t, uint32_t> CAmBot__LoadBotMarshal::m_BotBehaviorIds;

void __fastcall CAmBot__LoadBotMarshal::Call(void* CAmBotRow, void* edx, void* Marshal) {
	CallOriginal(CAmBotRow, edx, Marshal);

	m_BotPointers[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]] = (int)(CAmBotRow);
	m_BotBehaviorIds[CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]] = CMarshal__GetByte::m_values[GA_T::GA_T::BEHAVIOR_ID];

	if (bPopulateDatabase) {
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
				physical_type_value_id \
			) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, 0);

		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return;
		}

		if (result == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, CMarshal__GetByte::m_values[GA_T::GA_T::BOT_ID]);
			sqlite3_bind_text(stmt, 2, CMarshal__GetString2::m_values[GA_T::GA_T::REFERENCE_NAME], -1, SQLITE_STATIC);
			sqlite3_bind_int(stmt, 3, CMarshal__GetByte::m_values[GA_T::GA_T::NAME_MSG_ID]);
			sqlite3_bind_int(stmt, 4, CMarshal__GetByte::m_values[GA_T::GA_T::DESC_MSG_ID]);
			sqlite3_bind_int(stmt, 5, CMarshal__GetByte::m_values[GA_T::GA_T::LEVEL]);
			sqlite3_bind_int(stmt, 6, CMarshal__GetInt32t::m_values[GA_T::GA_T::PAWN_CLASS_RES_ID]);
			sqlite3_bind_int(stmt, 7, CMarshal__GetInt32t::m_values[GA_T::GA_T::CONTROLLER_CLASS_RES_ID]);
			sqlite3_bind_int(stmt, 8, CMarshal__GetByte::m_values[GA_T::GA_T::BEHAVIOR_ID]);
			sqlite3_bind_int(stmt, 9, CMarshal__GetByte::m_values[GA_T::GA_T::HEAD_ASM_ID]);
			sqlite3_bind_int(stmt, 10, CMarshal__GetByte::m_values[GA_T::GA_T::BODY_ASM_ID]);
			sqlite3_bind_int(stmt, 11, CMarshal__GetByte::m_values[GA_T::GA_T::MOVEMENT_ASM_ID]);
			sqlite3_bind_int(stmt, 12, CMarshal__GetByte::m_values[GA_T::GA_T::HIT_POINTS]);
			sqlite3_bind_int(stmt, 13, CMarshal__GetByte::m_values[GA_T::GA_T::BOT_TYPE_VALUE_ID]);
			sqlite3_bind_int(stmt, 14, CMarshal__GetByte::m_values[GA_T::GA_T::PHYSICAL_TYPE_VALUE_ID]);

			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}
}

