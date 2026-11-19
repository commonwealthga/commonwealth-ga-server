#include "src/Database/Database.hpp"
#include "src/Shared/SchemaVersion.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <map>
#include <string>
#include <vector>

sqlite3* Database::connection = nullptr;

sqlite3* Database::GetConnection() {
	if (Database::connection == nullptr) {
		const std::string db_path = Config::GetDbPath();
		int result = sqlite3_open(db_path.c_str(), &connection);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to open database '%s': %s", db_path.c_str(), sqlite3_errmsg(connection));
			return nullptr;
		}
		Logger::Log("db", "Opened database '%s'\n", db_path.c_str());
		// journal_mode is persistent in the DB header (set by the control-server
		// open) — re-asserting it is cheap and makes the intent explicit. The
		// other three pragmas are per-connection: every game-DLL connection has
		// to set them itself. journal_size_limit is the one that actually shrinks
		// the WAL after each autocheckpoint — without it the file grows to its
		// historical high-water mark and never comes back down.
		sqlite3_exec(connection, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
		sqlite3_exec(connection, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
		sqlite3_exec(connection, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
		sqlite3_exec(connection, "PRAGMA journal_size_limit=67108864;", nullptr, nullptr, nullptr);
	}

	return Database::connection;
}

void Database::CloseConnection() {
	if (Database::connection != nullptr) {
		sqlite3_exec(Database::connection, "PRAGMA wal_checkpoint(TRUNCATE);", nullptr, nullptr, nullptr);
	}
	sqlite3_close(Database::connection);
	Database::connection = nullptr;
}

int Database::Callback(void* data, int argc, char** argv, char** azColName) {

	std::vector<std::map<std::string, std::string>>* dataMap = static_cast<std::vector<std::map<std::string, std::string>>*>(data);

	std::map<std::string, std::string> row;

	for (int i = 0; i < argc; i++) {
		row[azColName[i]] = argv[i];
	}

	dataMap->push_back(row);

	return 0;
}

void Database::Init() {
	sqlite3* db = GetConnection();
	if (!db) {
		Logger::Log("db", "Database::Init skipped: no DB connection\n");
		PlayerRegistry::Init();
		return;
	}

	// This process does not migrate. The control server owns the schema: it
	// installs dbbase/base.db on a fresh database and applies migrations/*.sql
	// after it. Two processes migrating one version counter is exactly what
	// produced node databases stuck at a low version with the whole ladder
	// replayed against them on every instance start.
	//
	// All this side does is say so when the database is older than the code
	// expects, because a missing column otherwise surfaces as an obscure query
	// error in the middle of a match.
	int version = 0;
	{
		char* err = nullptr;
		std::vector<std::map<std::string, std::string>> data;
		if (sqlite3_exec(db, "SELECT max(version) AS v FROM version_info", Callback, &data,
		                 &err) == SQLITE_OK && !data.empty() && !data[0]["v"].empty()) {
			version = std::stoi(data[0]["v"]);
		}
		sqlite3_free(err);
	}
	if (version < Schema::kVersion) {
		Logger::Log("db",
			"WARNING: database schema is at %d, this build expects %d - start the "
			"control server once to migrate it\n",
			version, Schema::kVersion);
	}

	PlayerRegistry::Init();
}

std::string Database::GetAgencyName(int64_t character_id) {
	if (character_id <= 0) return "";
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	// Membership is per ACCOUNT; m.character_id is only a join-time snapshot,
	// so resolve through the character's user_id, not the snapshot.
	if (sqlite3_prepare_v2(db,
			"SELECT ag.name FROM ga_characters c "
			"JOIN ga_agency_members m ON m.user_id = c.user_id "
			"JOIN ga_agencies ag ON ag.id = m.agency_id "
			"WHERE c.id = ?",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return "";
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	std::string name;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		const char* a = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		if (a) name = a;
	}
	sqlite3_finalize(stmt);
	return name;
}

std::string Database::GetQuestStatus(int64_t character_id, int quest_id) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT status FROM ga_character_quests WHERE character_id = ? AND quest_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] GetQuestStatus prepare failed: %s\n", sqlite3_errmsg(db));
		return "";
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, quest_id);

	std::string status;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		const char* s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		if (s) status = s;
	}
	sqlite3_finalize(stmt);
	return status;
}

void Database::AcceptQuest(int64_t character_id, int quest_id) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO ga_character_quests (character_id, quest_id, status) VALUES (?, ?, 'active')",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] AcceptQuest prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, quest_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	Logger::Log("db", "[Quest] Accepted quest %d for character %lld\n", quest_id, character_id);
}

void Database::CompleteQuest(int64_t character_id, int quest_id) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_character_quests SET status = 'complete', completed_at = strftime('%s','now') "
		"WHERE character_id = ? AND quest_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] CompleteQuest prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, quest_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	Logger::Log("db", "[Quest] Completed quest %d for character %lld\n", quest_id, character_id);
}

void Database::AbandonQuest(int64_t character_id, int quest_id) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"DELETE FROM ga_character_quests WHERE character_id = ? AND quest_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] AbandonQuest prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, quest_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	Logger::Log("db", "[Quest] Abandoned quest %d for character %lld\n", quest_id, character_id);
}

