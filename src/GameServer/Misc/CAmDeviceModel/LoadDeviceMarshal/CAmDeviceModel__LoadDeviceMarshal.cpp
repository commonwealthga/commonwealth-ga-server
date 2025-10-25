#include "src/GameServer/Misc/CAmDeviceModel/LoadDeviceMarshal/CAmDeviceModel__LoadDeviceMarshal.hpp"
#include "src/GameServer/Misc/CMarshal/GetByte/CMarshal__GetByte.hpp"
#include "src/GameServer/Misc/CMarshal/GetInt32t/CMarshal__GetInt32t.hpp"
#include "src/GameServer/Misc/CMarshal/GetString2/CMarshal__GetString2.hpp"
#include "src/GameServer/Misc/CMarshal/GetFloat/CMarshal__GetFloat.hpp"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool CAmDeviceModel__LoadDeviceMarshal::bPopulateDatabaseDevices = false;

void __fastcall CAmDeviceModel__LoadDeviceMarshal::Call(void* CAmDeviceModelRow, void* edx, void* Marshal) {

	CallOriginal(CAmDeviceModelRow, edx, Marshal);

	if (bPopulateDatabaseDevices) {
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		int result = sqlite3_prepare_v2(db,
			"INSERT INTO asm_data_set_devices ( \
				device_id, \
				form_class_res_id, \
				mount_socket_res_id, \
				time_to_equip_secs, \
				container_skill_group_id, \
				right_click_behavior_type_value_id, \
				slot_used_value_id \
			) VALUES (?, ?, ?, ?, ?, ?, ?)", -1, &stmt, 0);

		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
			return;
		}

		sqlite3_bind_int(stmt, 1, CMarshal__GetInt32t::m_values[GA_T::GA_T::DEVICE_ID]);
		sqlite3_bind_int(stmt, 2, CMarshal__GetInt32t::m_values[GA_T::GA_T::FORM_CLASS_RES_ID]);
		sqlite3_bind_int(stmt, 3, CMarshal__GetInt32t::m_values[GA_T::GA_T::MOUNT_SOCKET_RES_ID]);
		sqlite3_bind_int(stmt, 4, CMarshal__GetFloat::m_values[GA_T::GA_T::TIME_TO_EQUIP_SECS]);
		sqlite3_bind_int(stmt, 5, CMarshal__GetByte::m_values[GA_T::GA_T::CONTAINER_SKILL_GROUP_ID]);
		sqlite3_bind_int(stmt, 6, CMarshal__GetByte::m_values[GA_T::GA_T::RIGHT_CLICK_BEHAVIOR_TYPE_VALUE_ID]);
		sqlite3_bind_int(stmt, 7, CMarshal__GetByte::m_values[GA_T::GA_T::SLOT_USED_VALUE_ID]);

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

}

