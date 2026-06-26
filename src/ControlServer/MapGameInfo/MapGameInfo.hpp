#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Process-wide read-only registry of map_game_info rows, populated once at
// startup. Each record carries the four fields GSC_GO_PLAY needs (plus the
// gameplay-type hint). Lookup is keyed by .upk filename — case-insensitive
// because some seeded names lowercase the leading word (e.g. `push_Ravine_P`).
//
// Schema (created by game-server migration v51 in the shared server.db):
//   map_game_id INTEGER PRIMARY KEY
//   map_name TEXT
//   game_class TEXT
//   gameplay_type_value_id INTEGER
//   friendly_name_msg_id INTEGER NOT NULL
//   entry_background_image_res_id INTEGER
struct MapGameRecord {
    uint32_t    map_game_id;
    std::string map_name;
    std::string game_class;
    uint32_t    gameplay_type_value_id;
    uint32_t    friendly_name_msg_id;
    uint32_t    entry_background_image_res_id;
    // Loading-screen vanity name resolved from friendly_name_msg_id via
    // asm_data_set_msg_translations (e.g. "CONTROL: Seaside"). Falls back to
    // map_name when there's no translation.
    std::string friendly_name;
};

class MapGameInfo {
public:
    // Re-loads the registry from DB. Safe to call before the schema exists
    // (e.g. fresh DB) — registry will simply be empty and lookups will miss.
    static void Init();

    // Returns the record for the given .upk filename if one exists.
    // Case-insensitive match against map_name.
    static std::optional<MapGameRecord> LookupByName(const std::string& map_name);

    // One challenge-playable map (PvP). `number` is the 1-based position in the
    // stable list — exactly what a player types as `<map>` in -challenge.
    struct ChallengeMapEntry {
        int         number = 0;
        std::string map_name;
        std::string game_class;     // = the instance game_mode
        std::string display_name;   // vanity/loading-screen name (falls back to map_name)
    };

    // PvP maps playable in a -challenge match: every map_game_info row with a
    // real map_name whose game_class is not the PvE TgGame_Mission, deduped by
    // name and ordered stably (by map_name) so a given number is consistent
    // between the list display and number→map resolution.
    static std::vector<ChallengeMapEntry> GetChallengeMaps();
};
