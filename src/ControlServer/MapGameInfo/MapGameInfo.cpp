#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace {

std::string LowerCopy(const std::string& s) {
    std::string out; out.reserve(s.size());
    for (char c : s) out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return out;
}

std::string StripReservedSuffix(std::string s) {
    constexpr const char* kSuffix = "_reserved";
    const size_t suffixLen = 9;
    if (s.size() >= suffixLen && s.compare(s.size() - suffixLen, suffixLen, kSuffix) == 0) {
        s.resize(s.size() - suffixLen);
    }
    return s;
}

// Keyed by lowercased map_name for case-insensitive lookup.
std::unordered_map<std::string, MapGameRecord> g_byMapName;

const char* SafeText(sqlite3_stmt* stmt, int col) {
    const unsigned char* t = sqlite3_column_text(stmt, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

}  // namespace

void MapGameInfo::Init() {
    g_byMapName.clear();

    sqlite3* db = Database::GetConnection();
    if (!db) {
        Logger::Log("config", "MapGameInfo::Init — DB connection unavailable\n");
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* kSql =
        "SELECT m.map_game_id, COALESCE(m.map_name, ''), COALESCE(m.game_class, ''), "
        "       COALESCE(m.gameplay_type_value_id, 0), m.friendly_name_msg_id, "
        "       COALESCE(m.entry_background_image_res_id, 0), "
        "       COALESCE(NULLIF(t.message, ''), m.map_name) "
        "FROM map_game_info m "
        "LEFT JOIN asm_data_set_msg_translations t ON t.msg_id = m.friendly_name_msg_id";
    if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        // Table may not exist yet (game-server v51 hasn't run). Treat as
        // empty registry — every lookup falls back to caller defaults.
        Logger::Log("config", "MapGameInfo::Init — prepare failed (%s); registry empty\n",
            sqlite3_errmsg(db));
        return;
    }

    int loadedRows = 0;
    int skippedNoName = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MapGameRecord r;
        r.map_game_id                   = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
        r.map_name                      = SafeText(stmt, 1);
        r.game_class                    = SafeText(stmt, 2);
        r.gameplay_type_value_id        = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
        r.friendly_name_msg_id          = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
        r.entry_background_image_res_id = static_cast<uint32_t>(sqlite3_column_int(stmt, 5));
        r.friendly_name                 = SafeText(stmt, 6);

        if (r.map_name.empty()) {
            // Rows whose map_name is still NULL can't be reverse-resolved by
            // the matchmaker; skip silently — they're carried in the DB for
            // future hand-curation but contribute nothing to the registry.
            skippedNoName++;
            continue;
        }
        const std::string key = LowerCopy(r.map_name);
        g_byMapName.emplace(key, r);
        g_byMapName.emplace(StripReservedSuffix(key), r);
        loadedRows++;
    }
    sqlite3_finalize(stmt);

    Logger::Log("config",
        "MapGameInfo::Init — loaded %d map_game_info rows (skipped %d with NULL map_name)\n",
        loadedRows, skippedNoName);
}

std::optional<MapGameRecord> MapGameInfo::LookupByName(const std::string& map_name) {
    auto it = g_byMapName.find(LowerCopy(map_name));
    if (it == g_byMapName.end()) return std::nullopt;
    return it->second;
}

std::vector<MapGameInfo::ChallengeCategory> MapGameInfo::GetChallengeCategories() {
    std::vector<ChallengeCategory> out;

    sqlite3* db = Database::GetConnection();
    if (!db) return out;

    sqlite3_stmt* stmt = nullptr;
    // Curated catalog. game_class is resolved from map_game_info (source of
    // truth) via a correlated subquery — NOCASE because some map_names lowercase
    // the leading word (e.g. push_Ravine_P). Catalog rows are already grouped by
    // category and ordered, so we just walk them and start a new category each
    // time category_pos changes.
    const char* kSql =
        "SELECT c.category, c.category_pos, c.vanity_name, c.map_name, "
        "       (SELECT m.game_class FROM map_game_info m "
        "        WHERE m.map_name = c.map_name COLLATE NOCASE "
        "          AND m.game_class IS NOT NULL AND m.game_class <> '' LIMIT 1) AS game_class "
        "FROM cs_challenge_catalog c "
        "ORDER BY c.category_pos, c.map_pos";
    if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        // Table may not exist yet (game-server v125 hasn't run). Empty catalog —
        // -challenge will just show no types until the migration runs.
        Logger::Log("config", "MapGameInfo::GetChallengeCategories — prepare failed (%s)\n",
            sqlite3_errmsg(db));
        return out;
    }

    int cur_pos = -1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const std::string category   = SafeText(stmt, 0);
        const int         categoryPos = sqlite3_column_int(stmt, 1);
        const std::string vanity     = SafeText(stmt, 2);
        const std::string map_name   = SafeText(stmt, 3);
        const std::string game_class = SafeText(stmt, 4);

        if (game_class.empty()) {
            // No map_game_info row → no game_mode to launch. Drop it rather than
            // risk spawning a broken instance.
            Logger::Log("config",
                "MapGameInfo::GetChallengeCategories — '%s' has no map_game_info game_class; skipped\n",
                map_name.c_str());
            continue;
        }

        if (categoryPos != cur_pos) {
            ChallengeCategory cat;
            cat.number = categoryPos;
            cat.name   = category;
            out.push_back(std::move(cat));
            cur_pos = categoryPos;
        }

        ChallengeMapEntry e;
        e.number       = static_cast<int>(out.back().maps.size()) + 1;
        e.map_name     = map_name;
        e.game_class   = game_class;
        e.display_name = vanity;
        out.back().maps.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return out;
}
