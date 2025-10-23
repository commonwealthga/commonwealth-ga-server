#include "src/GameServer/Misc/CAmBot/LoadBotSpawnTableMarshal/CAmBot__LoadBotSpawnTableMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmBot__LoadBotSpawnTableMarshal::bPopulateDatabase = false;

void __fastcall CAmBot__LoadBotSpawnTableMarshal::Call(void *param_1, void *edx, void *param_2, int param_3) {
	if (bPopulateDatabase) {
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		int result = sqlite3_prepare_v2(db,
			"INSERT INTO asm_data_set_bot_spawn_tables ( \
				bot_spawn_table_id, \
				difficulty_value_id, \
				player_profile_id, \
				spawn_group, \
				enemy_bot_id, \
				bot_count, \
				spawn_chance, \
				team_size, \
				multiple_class_flag, \
				bot_balance_multiplier, \
				spawn_group_min, \
				spawn_group_max, \
				spawn_group_respawn_sec \
			) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, 0);

		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return;
		}

		if (result == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, CMarshal__GetByte::m_values[GA_T::GA_T::BOT_SPAWN_TABLE_ID]);
			sqlite3_bind_int(stmt, 2, CMarshal__GetByte::m_values[GA_T::GA_T::DIFFICULTY_VALUE_ID]);
			sqlite3_bind_int(stmt, 3, CMarshal__GetByte::m_values[GA_T::GA_T::PLAYER_PROFILE_ID]);
			sqlite3_bind_int(stmt, 4, CMarshal__GetByte::m_values[GA_T::GA_T::SPAWN_GROUP]);
			sqlite3_bind_int(stmt, 5, CMarshal__GetByte::m_values[GA_T::GA_T::ENEMY_BOT_ID]);
			sqlite3_bind_int(stmt, 6, CMarshal__GetByte::m_values[GA_T::GA_T::BOT_COUNT]);
			sqlite3_bind_double(stmt, 7, CMarshal__GetFloat::m_values[GA_T::GA_T::SPAWN_CHANCE]);
			sqlite3_bind_int(stmt, 8, CMarshal__GetByte::m_values[GA_T::GA_T::TEAM_SIZE]);
			sqlite3_bind_int(stmt, 9, 0); //  CMarshal__GetFlag not implemented yet GA_T::GA_T::MULTIPLE_CLASS_FLAG
			sqlite3_bind_double(stmt, 10, CMarshal__GetFloat::m_values[GA_T::GA_T::BOT_BALANCE_MULTIPLIER]);
			sqlite3_bind_int(stmt, 11, CMarshal__GetByte::m_values[GA_T::GA_T::SPAWN_GROUP_MIN]);
			sqlite3_bind_int(stmt, 12, CMarshal__GetByte::m_values[GA_T::GA_T::SPAWN_GROUP_MAX]);
			sqlite3_bind_int(stmt, 13, CMarshal__GetByte::m_values[GA_T::GA_T::SPAWN_GROUP_RESPAWN_SEC]);

			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	CallOriginal(param_1, edx, param_2, param_3);
}

