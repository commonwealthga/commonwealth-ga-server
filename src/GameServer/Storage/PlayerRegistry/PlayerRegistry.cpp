#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "lib/sqlite3/sqlite3.h"
#include <cstdio>

std::map<std::string, PlayerInfo> PlayerRegistry::by_guid_;
std::map<std::string, PlayerInfo> PlayerRegistry::by_ip_;
uint64_t PlayerRegistry::next_generation_ = 1;
CRITICAL_SECTION PlayerRegistry::cs_;
bool PlayerRegistry::cs_initialized_ = false;

// RAII guard for CRITICAL_SECTION
struct CsGuard {
	CRITICAL_SECTION& cs;
	CsGuard(CRITICAL_SECTION& c) : cs(c) { EnterCriticalSection(&cs); }
	~CsGuard() { LeaveCriticalSection(&cs); }
};

void PlayerRegistry::EnsureInitialized() {
	if (!cs_initialized_) {
		InitializeCriticalSection(&cs_);
		cs_initialized_ = true;
	}
}

void PlayerRegistry::Init() {
	EnsureInitialized();
	CsGuard lock(cs_);
	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("db", "[PlayerRegistry] Init skipped: no DB connection\n");
		by_guid_.clear();
		by_ip_.clear();
		return;
	}
	char* err = nullptr;
	int rc = sqlite3_exec(db, "DELETE FROM ga_players", nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerRegistry] Failed to clear stale sessions: %s\n", err);
		sqlite3_free(err);
	} else {
		Logger::Log("db", "[PlayerRegistry] Cleared stale player sessions\n");
	}

	by_guid_.clear();
	by_ip_.clear();
}

void PlayerRegistry::Register(const PlayerInfo& info) {
	EnsureInitialized();
	Logger::Log("db",
		"[PlayerRegistry] Register begin name=%s guid=%s user=%lld char=%lld profile=%u\n",
		info.player_name.c_str(), info.session_guid.c_str(),
		(long long)info.user_id, (long long)info.selected_character_id,
		info.selected_profile_id);
	CsGuard lock(cs_);
	PlayerInfo stored = info;
	stored.register_generation = next_generation_++;

	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("db", "[PlayerRegistry] Register: no DB connection, keeping memory registry only\n");
		by_guid_[stored.session_guid] = stored;
		if (!stored.ip_address.empty()) by_ip_[stored.ip_address] = stored;
		return;
	}

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT OR REPLACE INTO ga_players "
		"(session_guid, player_name, ip_address, user_id, created_at, last_seen_at) "
		"VALUES (?, ?, ?, ?, strftime('%s','now'), strftime('%s','now'))",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerRegistry] Failed to prepare register: %s\n", sqlite3_errmsg(db));
	} else {
		sqlite3_bind_text(stmt,  1, stored.session_guid.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt,  2, stored.player_name.c_str(),  -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt,  3, stored.ip_address.c_str(),   -1, SQLITE_TRANSIENT);
		if (stored.user_id != 0)
			sqlite3_bind_int64(stmt, 4, stored.user_id);
		else
			sqlite3_bind_null(stmt, 4);
		rc = sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		if (rc != SQLITE_DONE)
			Logger::Log("db", "[PlayerRegistry] Failed to register player: %s\n", sqlite3_errmsg(db));
	}
	Logger::Log("db", "[PlayerRegistry] Register DB step done guid=%s rc=%d\n",
		info.session_guid.c_str(), rc);

	{
		by_guid_[stored.session_guid] = stored;
		if (!stored.ip_address.empty()) by_ip_[stored.ip_address] = stored;
	}
	Logger::Log("db", "[PlayerRegistry] Register memory step done guid=%s\n",
		stored.session_guid.c_str());

	Logger::Log("db", "[PlayerRegistry] Registered player '%s' guid=%s ip=%s generation=%llu\n",
		stored.player_name.c_str(), stored.session_guid.c_str(), stored.ip_address.c_str(),
		(unsigned long long)stored.register_generation);
}

void PlayerRegistry::Unregister(const std::string& session_guid) {
	EnsureInitialized();
	UnregisterIfGeneration(session_guid, 0);
}

bool PlayerRegistry::UnregisterIfGeneration(const std::string& session_guid,
                                            uint64_t register_generation) {
	EnsureInitialized();
	CsGuard lock(cs_);
	auto it_current = by_guid_.find(session_guid);
	if (register_generation != 0 &&
	    (it_current == by_guid_.end() ||
	     it_current->second.register_generation != register_generation)) {
		Logger::Log("db",
			"[PlayerRegistry] Skip stale unregister guid=%s generation=%llu current=%llu\n",
			session_guid.c_str(), (unsigned long long)register_generation,
			(unsigned long long)(it_current == by_guid_.end()
				? 0 : it_current->second.register_generation));
		return false;
	}

	sqlite3* db = Database::GetConnection();
	if (!db) {
		if (it_current != by_guid_.end() && !it_current->second.ip_address.empty()) {
			by_ip_.erase(it_current->second.ip_address);
		}
		by_guid_.erase(session_guid);
		return true;
	}

	char sql[256];
	snprintf(sql, sizeof(sql),
		"DELETE FROM ga_players WHERE session_guid='%s'",
		session_guid.c_str());

	char* err = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerRegistry] Failed to unregister player: %s\n", err);
		sqlite3_free(err);
	}

	{
		auto it = by_guid_.find(session_guid);
		if (it != by_guid_.end()) {
			by_ip_.erase(it->second.ip_address);
		}

		by_guid_.erase(session_guid);
	}

	Logger::Log("db", "[PlayerRegistry] Unregistered player guid=%s generation=%llu\n",
		session_guid.c_str(), (unsigned long long)register_generation);
	return true;
}

std::optional<PlayerInfo> PlayerRegistry::GetByGuid(const std::string& guid) {
	EnsureInitialized();
	CsGuard lock(cs_);
	auto it = by_guid_.find(guid);
	if (it != by_guid_.end()) return it->second;
	return std::nullopt;
}

PlayerInfo* PlayerRegistry::GetByGuidPtr(const std::string& guid) {
	EnsureInitialized();
	CsGuard lock(cs_);
	auto it = by_guid_.find(guid);
	if (it != by_guid_.end()) return &it->second;
	return nullptr;
}

std::optional<PlayerInfo> PlayerRegistry::GetByIp(const std::string& ip) {
	EnsureInitialized();
	CsGuard lock(cs_);
	auto it = by_ip_.find(ip);
	if (it != by_ip_.end()) return it->second;
	return std::nullopt;
}

int64_t PlayerRegistry::UpsertUser(const std::string& username) {
	EnsureInitialized();
	CsGuard lock(cs_);
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;

	// Insert if not present (ignore on conflict keeps existing row).
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO ga_users (username) VALUES (?)", -1, &stmt, nullptr);
	if (rc == SQLITE_OK && stmt) {
		sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	} else {
		Logger::Log("db", "[PlayerRegistry] UpsertUser insert prepare failed: %s\n", sqlite3_errmsg(db));
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
		Logger::Log("db", "[PlayerRegistry] UpsertUser select prepare failed: %s\n", sqlite3_errmsg(db));
	}

	return id;
}

int64_t PlayerRegistry::InsertCharacter(int64_t user_id, uint32_t profile_id,
                                        uint32_t head_asm_id, uint32_t gender_type_value_id,
                                        const std::vector<uint8_t>& morph_data) {
	EnsureInitialized();
	CsGuard lock(cs_);
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_characters (user_id, profile_id, head_asm_id, gender_type_value_id, morph_data) "
		"VALUES (?, ?, ?, ?, ?)",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerRegistry] InsertCharacter prepare failed: %s\n", sqlite3_errmsg(db));
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

	return sqlite3_last_insert_rowid(db);
}

std::vector<CharacterInfo> PlayerRegistry::GetCharactersByUserId(int64_t user_id) {
	EnsureInitialized();
	CsGuard lock(cs_);
	sqlite3* db = Database::GetConnection();
	std::vector<CharacterInfo> result;
	if (!db) return result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, profile_id, head_asm_id, gender_type_value_id, morph_data, "
		"       hair_asm_id, skin_mat_param_id, eye_mat_param_id "
		"FROM ga_characters WHERE user_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerRegistry] GetCharactersByUserId prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, user_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		CharacterInfo c;
		c.id                  = sqlite3_column_int64(stmt, 0);
		c.user_id             = user_id;
		c.profile_id          = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));
		c.head_asm_id         = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));
		c.gender_type_value_id = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
		const void* blob = sqlite3_column_blob(stmt, 4);
		int bytes        = sqlite3_column_bytes(stmt, 4);
		if (blob && bytes > 0)
			c.morph_data.assign(static_cast<const uint8_t*>(blob),
			                    static_cast<const uint8_t*>(blob) + bytes);
		c.hair_asm_id         = static_cast<uint32_t>(sqlite3_column_int(stmt, 5));
		c.skin_mat_param_id   = static_cast<uint32_t>(sqlite3_column_int(stmt, 6));
		c.eye_mat_param_id    = static_cast<uint32_t>(sqlite3_column_int(stmt, 7));
		result.push_back(std::move(c));
	}
	sqlite3_finalize(stmt);
	return result;
}

std::optional<CharacterInfo> PlayerRegistry::GetCharacterById(int64_t id) {
	EnsureInitialized();
	CsGuard lock(cs_);
	sqlite3* db = Database::GetConnection();
	if (!db) return std::nullopt;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, user_id, profile_id, head_asm_id, gender_type_value_id, morph_data, "
		"       hair_asm_id, skin_mat_param_id, eye_mat_param_id "
		"FROM ga_characters WHERE id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerRegistry] GetCharacterById prepare failed: %s\n", sqlite3_errmsg(db));
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
		c.hair_asm_id          = static_cast<uint32_t>(sqlite3_column_int(stmt, 6));
		c.skin_mat_param_id    = static_cast<uint32_t>(sqlite3_column_int(stmt, 7));
		c.eye_mat_param_id     = static_cast<uint32_t>(sqlite3_column_int(stmt, 8));
		result = std::move(c);
	}
	sqlite3_finalize(stmt);
	return result;
}

void PlayerRegistry::SetSelectedCharacter(const std::string& session_guid, int64_t char_id, uint32_t profile_id) {
	EnsureInitialized();
	CsGuard lock(cs_);
	auto it = by_guid_.find(session_guid);
	if (it != by_guid_.end()) {
		it->second.selected_character_id = char_id;
		it->second.selected_profile_id   = profile_id;
		if (!it->second.ip_address.empty()) {
			by_ip_[it->second.ip_address].selected_character_id = char_id;
			by_ip_[it->second.ip_address].selected_profile_id   = profile_id;
		}
	}
}

int PlayerRegistry::RefreshSkillsForProfile(const std::string& session_guid,
                                            int item_profile_id) {
	EnsureInitialized();
	if (item_profile_id < 1 || item_profile_id > 5) return 0;
	CsGuard lock(cs_);
	auto it = by_guid_.find(session_guid);
	if (it == by_guid_.end()) return 0;
	PlayerInfo& info = it->second;
	info.skills.clear();

	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT skill_group_id, skill_id, points "
			"FROM ga_character_skills "
			"WHERE character_id = ? AND item_profile_id = ? AND points > 0",
			-1, &st, nullptr) != SQLITE_OK) {
		Logger::Log("loadout",
			"[RefreshSkillsForProfile] prepare failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_bind_int64(st, 1, info.selected_character_id);
	sqlite3_bind_int  (st, 2, item_profile_id);
	int n = 0;
	while (sqlite3_step(st) == SQLITE_ROW) {
		SkillAllocation a;
		a.skill_group_id = sqlite3_column_int(st, 0);
		a.skill_id       = sqlite3_column_int(st, 1);
		a.points         = sqlite3_column_int(st, 2);
		info.skills.push_back(a);
		++n;
	}
	sqlite3_finalize(st);

	// Mirror to by_ip_ for any consumers reading from there.
	if (!info.ip_address.empty()) by_ip_[info.ip_address].skills = info.skills;

	Logger::Log("loadout",
		"[RefreshSkillsForProfile] guid=%s char=%lld itemProf=%d loaded=%d skill rows\n",
		session_guid.c_str(), (long long)info.selected_character_id, item_profile_id, n);
	return n;
}

