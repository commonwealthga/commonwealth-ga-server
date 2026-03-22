#pragma once

#include <string>
#include <cstdint>

class QuestStore {
public:
    std::string GetStatus(int64_t character_id, int quest_id);
    void Accept(int64_t character_id, int quest_id);
    void Complete(int64_t character_id, int quest_id);
    void Abandon(int64_t character_id, int quest_id);
};
