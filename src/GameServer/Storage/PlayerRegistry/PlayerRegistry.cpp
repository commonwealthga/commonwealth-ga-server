#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cstdio>

std::map<std::string, PlayerInfo> PlayerRegistry::by_guid_;
std::map<std::string, PlayerInfo> PlayerRegistry::by_ip_;

void PlayerRegistry::Init() {
	sqlite3* db = Database::GetConnection();
	char* err = nullptr;
	int rc = sqlite3_exec(db, "DELETE FROM ga_players", nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerRegistry] Failed to clear stale sessions: %s\n", err);
		sqlite3_free(err);
	} else {
		Logger::Log("db", "[PlayerRegistry] Cleared stale player sessions\n");
	}

	by_guid_.clear();
}

void PlayerRegistry::Register(const PlayerInfo& info) {
	sqlite3* db = Database::GetConnection();

	char sql[512];
	snprintf(sql, sizeof(sql),
		"INSERT OR REPLACE INTO ga_players "
		"(session_guid, player_name, ip_address, created_at, last_seen_at) "
		"VALUES ('%s', '%s', '%s', strftime('%%s','now'), strftime('%%s','now'))",
		info.session_guid.c_str(),
		info.player_name.c_str(),
		info.ip_address.c_str());

	char* err = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerRegistry] Failed to register player: %s\n", err);
		sqlite3_free(err);
	}

	{
		by_guid_[info.session_guid] = info;
		by_ip_[info.ip_address] = info;
	}

	Logger::Log("db", "[PlayerRegistry] Registered player '%s' guid=%s ip=%s\n",
		info.player_name.c_str(), info.session_guid.c_str(), info.ip_address.c_str());
}

void PlayerRegistry::Unregister(const std::string& session_guid) {
	sqlite3* db = Database::GetConnection();

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

	Logger::Log("db", "[PlayerRegistry] Unregistered player guid=%s\n", session_guid.c_str());
}

std::optional<PlayerInfo> PlayerRegistry::GetByGuid(const std::string& guid) {
	auto it = by_guid_.find(guid);
	if (it != by_guid_.end()) return it->second;
	return std::nullopt;
}

std::optional<PlayerInfo> PlayerRegistry::GetByIp(const std::string& ip) {
	auto it = by_ip_.find(ip);
	if (it != by_ip_.end()) return it->second;
	return std::nullopt;
}

