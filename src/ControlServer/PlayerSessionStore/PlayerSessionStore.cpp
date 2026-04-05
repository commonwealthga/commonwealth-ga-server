#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <cstdio>

std::mutex PlayerSessionStore::mutex_;
std::map<std::string, SessionInfo> PlayerSessionStore::by_guid_;
std::map<std::string, SessionInfo> PlayerSessionStore::by_ip_;

void PlayerSessionStore::Init() {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	char* err = nullptr;
	int rc = sqlite3_exec(db, "DELETE FROM ga_players", nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] Failed to clear stale sessions: %s\n", err);
		sqlite3_free(err);
	} else {
		Logger::Log("db", "[PlayerSessionStore] Cleared stale player sessions\n");
	}

	by_guid_.clear();
}

void PlayerSessionStore::Register(const SessionInfo& info) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT OR REPLACE INTO ga_players "
		"(session_guid, player_name, ip_address, user_id, created_at, last_seen_at) "
		"VALUES (?, ?, ?, ?, strftime('%s','now'), strftime('%s','now'))",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] Failed to prepare register: %s\n", sqlite3_errmsg(db));
	} else {
		sqlite3_bind_text(stmt, 1, info.session_guid.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, info.player_name.c_str(),  -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, info.ip_address.c_str(),   -1, SQLITE_TRANSIENT);
		if (info.user_id != 0)
			sqlite3_bind_int64(stmt, 4, info.user_id);
		else
			sqlite3_bind_null(stmt, 4);
		rc = sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		if (rc != SQLITE_DONE)
			Logger::Log("db", "[PlayerSessionStore] Failed to register player: %s\n", sqlite3_errmsg(db));
	}

	by_guid_[info.session_guid] = info;
	by_ip_[info.ip_address] = info;

	Logger::Log("db", "[PlayerSessionStore] Registered player '%s' guid=%s ip=%s\n",
		info.player_name.c_str(), info.session_guid.c_str(), info.ip_address.c_str());
}

void PlayerSessionStore::Unregister(const std::string& session_guid) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	char sql[256];
	snprintf(sql, sizeof(sql),
		"DELETE FROM ga_players WHERE session_guid='%s'",
		session_guid.c_str());

	char* err = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] Failed to unregister player: %s\n", err);
		sqlite3_free(err);
	}

	auto it = by_guid_.find(session_guid);
	if (it != by_guid_.end()) {
		by_ip_.erase(it->second.ip_address);
	}
	by_guid_.erase(session_guid);

	Logger::Log("db", "[PlayerSessionStore] Unregistered player guid=%s\n", session_guid.c_str());
}

std::optional<SessionInfo> PlayerSessionStore::GetByGuid(const std::string& guid) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = by_guid_.find(guid);
	if (it != by_guid_.end()) return it->second;
	return std::nullopt;
}

SessionInfo* PlayerSessionStore::GetByGuidPtr(const std::string& guid) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = by_guid_.find(guid);
	if (it != by_guid_.end()) return &it->second;
	return nullptr;
}

std::optional<SessionInfo> PlayerSessionStore::GetByIp(const std::string& ip) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = by_ip_.find(ip);
	if (it != by_ip_.end()) return it->second;
	return std::nullopt;
}

int64_t PlayerSessionStore::UpsertUser(const std::string& username) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	// Insert if not present (ignore on conflict keeps existing row).
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO ga_users (username) VALUES (?)", -1, &stmt, nullptr);
	if (rc == SQLITE_OK && stmt) {
		sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	} else {
		Logger::Log("db", "[PlayerSessionStore] UpsertUser insert prepare failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}

	// Fetch id.
	int64_t id = 0;
	rc = sqlite3_prepare_v2(db, "SELECT id FROM ga_users WHERE username = ?", -1, &stmt, nullptr);
	if (rc == SQLITE_OK && stmt) {
		sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW)
			id = sqlite3_column_int64(stmt, 0);
		sqlite3_finalize(stmt);
	} else {
		Logger::Log("db", "[PlayerSessionStore] UpsertUser select prepare failed: %s\n", sqlite3_errmsg(db));
	}

	return id;
}

int64_t PlayerSessionStore::InsertCharacter(int64_t user_id, uint32_t profile_id,
                                             uint32_t head_asm_id, uint32_t gender_type_value_id,
                                             const std::vector<uint8_t>& morph_data) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_characters (user_id, profile_id, head_asm_id, gender_type_value_id, morph_data) "
		"VALUES (?, ?, ?, ?, ?)",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] InsertCharacter prepare failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int(stmt,   2, static_cast<int>(profile_id));
	sqlite3_bind_int(stmt,   3, static_cast<int>(head_asm_id));
	sqlite3_bind_int(stmt,   4, static_cast<int>(gender_type_value_id));
	if (!morph_data.empty())
		sqlite3_bind_blob(stmt, 5, morph_data.data(), static_cast<int>(morph_data.size()), SQLITE_TRANSIENT);
	else
		sqlite3_bind_null(stmt, 5);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	int64_t newId = sqlite3_last_insert_rowid(db);
	SeedDefaultDevices(newId, profile_id);
	return newId;
}

std::vector<CharacterInfo> PlayerSessionStore::GetCharactersByUserId(int64_t user_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	std::vector<CharacterInfo> result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, profile_id, head_asm_id, gender_type_value_id, morph_data "
		"FROM ga_characters WHERE user_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetCharactersByUserId prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, user_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		CharacterInfo c;
		c.id                   = sqlite3_column_int64(stmt, 0);
		c.user_id              = user_id;
		c.profile_id           = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));
		c.head_asm_id          = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));
		c.gender_type_value_id = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
		const void* blob = sqlite3_column_blob(stmt, 4);
		int bytes        = sqlite3_column_bytes(stmt, 4);
		if (blob && bytes > 0)
			c.morph_data.assign(static_cast<const uint8_t*>(blob),
			                    static_cast<const uint8_t*>(blob) + bytes);
		result.push_back(std::move(c));
	}
	sqlite3_finalize(stmt);
	return result;
}

std::optional<CharacterInfo> PlayerSessionStore::GetCharacterById(int64_t id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, user_id, profile_id, head_asm_id, gender_type_value_id, morph_data "
		"FROM ga_characters WHERE id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetCharacterById prepare failed: %s\n", sqlite3_errmsg(db));
		return std::nullopt;
	}
	sqlite3_bind_int64(stmt, 1, id);

	std::optional<CharacterInfo> result;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		CharacterInfo c;
		c.id                   = sqlite3_column_int64(stmt, 0);
		c.user_id              = sqlite3_column_int64(stmt, 1);
		c.profile_id           = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));
		c.head_asm_id          = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
		c.gender_type_value_id = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
		const void* blob = sqlite3_column_blob(stmt, 5);
		int bytes        = sqlite3_column_bytes(stmt, 5);
		if (blob && bytes > 0)
			c.morph_data.assign(static_cast<const uint8_t*>(blob),
			                    static_cast<const uint8_t*>(blob) + bytes);
		result = std::move(c);
	}
	sqlite3_finalize(stmt);
	return result;
}

void PlayerSessionStore::SetSelectedCharacter(const std::string& session_guid, int64_t char_id, uint32_t profile_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = by_guid_.find(session_guid);
	if (it != by_guid_.end()) {
		it->second.selected_character_id = char_id;
		it->second.selected_profile_id   = profile_id;
		by_ip_[it->second.ip_address].selected_character_id = char_id;
		by_ip_[it->second.ip_address].selected_profile_id   = profile_id;
	}
}

int PlayerSessionStore::GetEffectGroupId(int deviceId) {
	switch (deviceId) {
		case 7031: return 26173;  // assault jetpack
		case 7032: return 26173;  // medic jetpack
		case 7033: return 26173;  // recon jetpack
		case 7034: return 26173;  // robotic jetpack
		case 2991: return 16670;  // agonizer
		case 2531: return 16653;  // healing grenade
		case 5800: return 22334;  // lifestealer
		case 2906: return 9071;   // adrenaline gun / specialty
		default:   return 0;
	}
}

void PlayerSessionStore::SeedDefaultDevices(int64_t character_id, uint32_t profile_id) {
	struct Seed { int deviceId; int slot; int slotValueId; int quality; };

	std::vector<Seed> seeds;
	switch (profile_id) {
		case 567: // Medic
			seeds = {
				{5800, 1, 221, 0}, {2991, 2, 198, 0}, {6898, 3, 200, 0}, {7032, 5, 201, 0},
				{2531, 7, 203, 0}, {2168, 8, 204, 0}, {2773, 10, 386, 0}, {864, 14, 502, 0},
			};
			break;
		case 679: // Recon
			seeds = {
				{5799, 1, 221, 0}, {2110, 2, 198, 0}, {3023, 3, 200, 0}, {7033, 5, 201, 0},
				{2219, 7, 203, 0}, {2129, 8, 204, 0}, {2113, 10, 386, 0}, {864, 14, 502, 0},
			};
			break;
		case 680: // Robotic
			seeds = {
				{5802, 1, 221, 0}, {6885, 2, 198, 0}, {2918, 3, 200, 0}, {7034, 5, 201, 0},
				{2300, 7, 203, 0}, {2066, 8, 204, 0}, {2886, 10, 386, 0}, {864, 14, 502, 0},
			};
			break;
		case 681: // Assault
		default:
			seeds = {
				{5801, 1, 221, 0}, {5788, 2, 198, 0}, {2914, 3, 200, 0}, {7031, 5, 201, 0},
				{3699, 7, 203, 0}, {2498, 8, 204, 0}, {5775, 10, 386, 0}, {864, 14, 502, 0},
			};
			break;
	}

	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_character_devices "
		"(character_id, device_id, equip_slot, slot_value_id, quality, inventory_id, effect_group_id) "
		"VALUES (?, ?, ?, ?, ?, ?, ?)",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] SeedDefaultDevices prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}

	int invId = 10000;
	for (const auto& s : seeds) {
		sqlite3_reset(stmt);
		sqlite3_bind_int64(stmt, 1, character_id);
		sqlite3_bind_int(stmt,   2, s.deviceId);
		sqlite3_bind_int(stmt,   3, s.slot);
		sqlite3_bind_int(stmt,   4, s.slotValueId);
		sqlite3_bind_int(stmt,   5, s.quality);
		sqlite3_bind_int(stmt,   6, invId++);
		sqlite3_bind_int(stmt,   7, GetEffectGroupId(s.deviceId));
		sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);

	Logger::Log("db", "[PlayerSessionStore] Seeded %d default devices for character %lld (profile %u)\n",
		(int)seeds.size(), character_id, profile_id);
}

std::vector<DeviceRow> PlayerSessionStore::GetDevicesForCharacter(int64_t character_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	std::vector<DeviceRow> result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT device_id, equip_slot, slot_value_id, quality, inventory_id, effect_group_id "
		"FROM ga_character_devices WHERE character_id = ? ORDER BY equip_slot",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetDevicesForCharacter prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, character_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		DeviceRow row;
		row.device_id       = sqlite3_column_int(stmt, 0);
		row.equip_slot      = sqlite3_column_int(stmt, 1);
		row.slot_value_id   = sqlite3_column_int(stmt, 2);
		row.quality         = sqlite3_column_int(stmt, 3);
		row.inventory_id    = sqlite3_column_int(stmt, 4);
		row.effect_group_id = sqlite3_column_int(stmt, 5);
		result.push_back(row);
	}
	sqlite3_finalize(stmt);
	return result;
}
