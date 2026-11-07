#include "src/ControlServer/QuestStore/QuestStore.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include <sqlite3.h>

std::string QuestStore::GetStatus(int64_t character_id, int quest_id) {
    return Database::GetQuestStatus(character_id, quest_id);
}

void QuestStore::Accept(int64_t character_id, int quest_id) {
    Database::AcceptQuest(character_id, quest_id);
}

void QuestStore::Complete(int64_t character_id, int quest_id) {
    Database::CompleteQuest(character_id, quest_id);
}

void QuestStore::Abandon(int64_t character_id, int quest_id) {
    Database::AbandonQuest(character_id, quest_id);
    // Abandon drops the step counters too, so taking the quest again starts at 0
    // (Collect steps read bag stock and are unaffected).
    if (sqlite3* db = Database::GetConnection()) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db, "DELETE FROM ga_character_quest_progress WHERE character_id = ? AND quest_id = ?",
                               -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(s, 1, character_id);
            sqlite3_bind_int(s, 2, quest_id);
            sqlite3_step(s);
        }
        sqlite3_finalize(s);
    }
}
