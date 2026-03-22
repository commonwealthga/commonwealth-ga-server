#include "src/ControlServer/QuestStore/QuestStore.hpp"
#include "src/ControlServer/Database/Database.hpp"

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
}
