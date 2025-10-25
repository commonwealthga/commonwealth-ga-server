#include "src/GameServer/Misc/CAmDeviceModel/LoadDeviceModeMarshal/CAmDeviceModel__LoadDeviceModeMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Misc/CMarshal/Translate/CMarshal__Translate.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmDeviceModel__LoadDeviceModeMarshal::bPopulateDatabaseDeviceModes = false;

void __fastcall CAmDeviceModel__LoadDeviceModeMarshal::Call(void* CAmDeviceModelRow, void* edx, void* Marshal) {

	CallOriginal(CAmDeviceModelRow, edx, Marshal);

	if (bPopulateDatabaseDeviceModes) {
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		int result = sqlite3_prepare_v2(db,
			"INSERT INTO asm_data_set_devices_data_set_device_modes ( \
				device_id, \
				device_mode_id, \
				name_msg_id, \
				name_msg_translated \
			) VALUES (?, ?, ?, ?)", -1, &stmt, 0);

		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return;
		}

		sqlite3_bind_int(stmt, 1, CMarshal__GetByte::m_values[GA_T::GA_T::DEVICE_ID]);
		sqlite3_bind_int(stmt, 2, CMarshal__GetByte::m_values[GA_T::GA_T::DEVICE_MODE_ID]);
		sqlite3_bind_int(stmt, 3, CMarshal__GetInt32t::m_values[GA_T::GA_T::NAME_MSG_ID]);
		sqlite3_bind_text(stmt, 4, CMarshal__Translate::m_values[GA_T::GA_T::NAME_MSG_ID], -1, SQLITE_STATIC);

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

}

