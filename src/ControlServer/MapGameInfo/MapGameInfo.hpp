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

    // One challenge-playable map (PvP). `number` is the 1-based position WITHIN
    // its category — exactly what a player types as `<map#>` in -challenge.
    struct ChallengeMapEntry {
        int         number = 0;
        std::string map_name;
        std::string game_class;     // = the instance game_mode
        std::string display_name;   // curated vanity name (cs_challenge_catalog)
    };

    // A player-facing -challenge category (Arena, Scramble, …). `number` is the
    // 1-based type number (category_pos) the player types as `<type>`.
    struct ChallengeCategory {
        int         number = 0;
        std::string name;                     // "Arena", "Scramble", …
        std::vector<ChallengeMapEntry> maps;  // ordered by map_pos; display = vanity
    };

    // The curated -challenge catalog from cs_challenge_catalog (seeded by
    // game-server migration v125), joined to map_game_info for game_class (the
    // source of truth). Categories are returned in category_pos order; maps
    // within each in map_pos order. A catalog row whose map_name has no
    // map_game_info match is skipped (logged) so it can never spawn a bad mode.
    static std::vector<ChallengeCategory> GetChallengeCategories();
};
