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
        "SELECT map_game_id, COALESCE(map_name, ''), COALESCE(game_class, ''), "
        "       COALESCE(gameplay_type_value_id, 0), friendly_name_msg_id, "
        "       COALESCE(entry_background_image_res_id, 0) "
        "FROM map_game_info";
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
