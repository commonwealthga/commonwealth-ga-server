#pragma once
#include <cstdint>

// Quest turn-in location check: a turn-in is expected right after the player
// used the quest's turn-in volume (the NPC). Mode from cs_settings key
// 'quest_turnin_check': "off", "log" (default: note mismatches only) or
// "enforce" (refuse the turn-in).
namespace TurninCheck {
    void NoteVolumeUse(int64_t character_id, int ui_volume_id);
    // true = allow. Logs every mismatch; refuses only in "enforce" mode.
    bool Allow(int64_t character_id, int quest_id);
}
