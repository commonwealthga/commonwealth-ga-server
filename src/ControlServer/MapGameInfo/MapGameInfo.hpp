#pragma once
#include <cstdint>
#include <optional>
#include <string>

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
};

class MapGameInfo {
public:
    // Re-loads the registry from DB. Safe to call before the schema exists
    // (e.g. fresh DB) — registry will simply be empty and lookups will miss.
    static void Init();

    // Returns the record for the given .upk filename if one exists.
    // Case-insensitive match against map_name.
    static std::optional<MapGameRecord> LookupByName(const std::string& map_name);
};
