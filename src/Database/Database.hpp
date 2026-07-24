#pragma once

#include "lib/sqlite3/sqlite3.h"
#include <cstdint>
#include <string>

class Database {
private:
	static sqlite3* connection;
public:
	static int Callback(void* data, int argc, char** argv, char** azColName);
	static sqlite3* GetConnection();
	static void CloseConnection();
	static void Init();

	// Agency name for a character, "" when unaffiliated. Feeds the in-match
	// scoreboard's Agency column (ATgRepInfo_Player::r_sAgencyName).
	static std::string GetAgencyName(int64_t character_id);

	// Returns "" (not found), "active", or "complete"
	static std::string GetQuestStatus(int64_t character_id, int quest_id);
	static void AcceptQuest(int64_t character_id, int quest_id);
	static void CompleteQuest(int64_t character_id, int quest_id);
	static void AbandonQuest(int64_t character_id, int quest_id);
};

