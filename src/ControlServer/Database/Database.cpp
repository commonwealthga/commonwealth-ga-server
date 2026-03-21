// Placeholder — will be replaced by Task 2
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <vector>
#include <map>
#include <string>

sqlite3* Database::connection = nullptr;
std::string Database::db_path_ = "server.db";

void Database::SetDbPath(const std::string& path) { db_path_ = path; }

sqlite3* Database::GetConnection() {
    if (Database::connection == nullptr) {
        int result = sqlite3_open(db_path_.c_str(), &connection);
        if (result != SQLITE_OK) {
            Logger::Log("db", "Failed to open database: %s\n", sqlite3_errmsg(connection));
            return nullptr;
        }
        sqlite3_exec(connection, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(connection, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
        sqlite3_exec(connection, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
    }
    return Database::connection;
}

void Database::CloseConnection() {
    sqlite3_close(Database::connection);
    Database::connection = nullptr;
}

int Database::Callback(void* data, int argc, char** argv, char** azColName) {
    auto* dataMap = static_cast<std::vector<std::map<std::string, std::string>>*>(data);
    std::map<std::string, std::string> row;
    for (int i = 0; i < argc; i++) {
        row[azColName[i]] = argv[i] ? argv[i] : "";
    }
    dataMap->push_back(row);
    return 0;
}

void Database::Init() {
    Logger::Log("db", "[Database::Init] stub -- full implementation in Task 2\n");
}

std::string Database::GetQuestStatus(int64_t, int) { return ""; }
void Database::AcceptQuest(int64_t, int) {}
void Database::CompleteQuest(int64_t, int) {}
void Database::AbandonQuest(int64_t, int) {}
