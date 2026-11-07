#include "src/ControlServer/QuestStore/TurninCheck.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <utility>

namespace TurninCheck {
namespace {
std::mutex g_mu;
std::map<int64_t, std::pair<int, int64_t>> g_last;  // char -> (volume, unix s)
constexpr int64_t kWindowSecs = 600;

int64_t Now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string Mode(sqlite3* db) {
    std::string mode = "log";
    sqlite3_stmt* s = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = 'quest_turnin_check'",
                                 -1, &s, nullptr) == SQLITE_OK) {
        if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_text(s, 0))
            mode = (const char*)sqlite3_column_text(s, 0);
        sqlite3_finalize(s);
    }
    return mode;
}
}  // namespace

void NoteVolumeUse(int64_t character_id, int ui_volume_id) {
    if (character_id <= 0 || ui_volume_id <= 0) return;
    std::lock_guard<std::mutex> lock(g_mu);
    g_last[character_id] = {ui_volume_id, Now()};
}

bool Allow(int64_t character_id, int quest_id) {
    sqlite3* db = Database::GetConnection();
    const std::string mode = Mode(db);
    if (mode == "off" || !db) return true;
    int turnin = 0;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COALESCE(turnin_ui_volume_id, 0) FROM asm_data_set_quests WHERE quest_id = ?",
                           -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, quest_id);
        if (sqlite3_step(s) == SQLITE_ROW) turnin = sqlite3_column_int(s, 0);
        sqlite3_finalize(s);
    }
    if (turnin <= 0) return true;  // no turn-in NPC: finishes on its own
    int last_vol = 0; int64_t age = -1;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        auto it = g_last.find(character_id);
        if (it != g_last.end()) { last_vol = it->second.first; age = Now() - it->second.second; }
    }
    if (last_vol == turnin && age >= 0 && age <= kWindowSecs) return true;
    const bool refuse = mode == "enforce";
    Logger::Log("quest", "[TurninCheck] char=%lld quest=%d turn-in volume %d, last used %d (%llds ago) -> %s\n",
                (long long)character_id, quest_id, turnin, last_vol, (long long)age,
                refuse ? "REFUSED" : "allowed (log mode)");
    return !refuse;
}
}  // namespace TurninCheck
