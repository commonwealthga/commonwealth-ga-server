#include "DeviceIds.hpp"
#include "../Database/Database.hpp"
#include "../Logger.hpp"
#include "sqlite3.h"

namespace GA {
namespace DeviceId {

int Lookup(const std::string& deviceName) {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        Logger::Log("deviceids", "[DeviceId::Lookup] No database connection\n");
        return 0;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* query =
        "SELECT i.item_id "
        "FROM asm_data_set_items i "
        "JOIN asm_data_set_devices d ON i.item_id = d.device_id "
        "WHERE i.name_msg_translated = ? "
        "LIMIT 1";

    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("deviceids", "[DeviceId::Lookup] prepare failed: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, deviceName.c_str(), -1, SQLITE_STATIC);

    int deviceId = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        deviceId = sqlite3_column_int(stmt, 0);
    } else {
        Logger::Log("deviceids", "[DeviceId::Lookup] device not found: '%s'\n", deviceName.c_str());
    }

    sqlite3_finalize(stmt);
    return deviceId;
}

} // namespace DeviceId
} // namespace GA
