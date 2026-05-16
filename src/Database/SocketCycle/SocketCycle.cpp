#include "src/Database/SocketCycle/SocketCycle.hpp"
#include "src/Database/Database.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <string>

namespace {

    // Cache key. asm_id fits well below 2^31; equip_point is a byte (1..24).
    // Pack into a single 64-bit so we can use unordered_map directly.
    static inline uint64_t MakeKey(int asm_id, int equip_point) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(asm_id)) << 32)
             | static_cast<uint32_t>(equip_point);
    }

    // Lazy FName slot: holds the source string from the DB and the
    // engine FName once we've converted it. `resolved` is the gating
    // bit; we never call FName(char*) before the engine has populated
    // GNames, so all conversion happens on first runtime access.
    struct LazyFName {
        std::string  src;
        FName        cached;
        bool         resolved = false;
    };

    // (asm_id, equip_point) → ordered list of "*Origin*" socket names.
    // Indexed by display_order - 1 (so m_nSocketIndex K maps to index K-1).
    static std::unordered_map<uint64_t, std::vector<LazyFName>> g_OriginCycle;

    // bot_id → body_asm_id, populated lazily from asm_data_set_bots.
    static std::unordered_map<int, int> g_BotAsmCache;

    static int LookupBodyAsmId(int botId) {
        auto it = g_BotAsmCache.find(botId);
        if (it != g_BotAsmCache.end()) return it->second;

        int asmId = 0;
        sqlite3* db = Database::GetConnection();
        if (db) {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                "SELECT body_asm_id FROM asm_data_set_bots WHERE bot_id = ? LIMIT 1;",
                -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, botId);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    asmId = sqlite3_column_int(stmt, 0);
                }
                sqlite3_finalize(stmt);
            }
        }
        g_BotAsmCache[botId] = asmId;
        return asmId;
    }

    // Forward mapping (slot_value_id → equip_point) lifted from
    // TgGame__SpawnBotById.cpp's BotGetEquipPointBySlot. Kept inline here
    // to avoid coupling on a sibling translation unit and to make the
    // dependency explicit at the only place that needs it.
    static inline int EquipPointFromSlotValueId(int slotUsedValueId) {
        switch (slotUsedValueId) {
            case 221: return 1;  case 198: return 2;  case 200: return 3;
            case 199: return 4;  case 201: return 5;  case 202: return 6;
            case 203: return 7;  case 204: return 8;  case 385: return 9;
            case 386: return 10; case 499: return 11; case 500: return 12;
            case 501: return 13; case 502: return 14; case 823: return 15;
            case 996: return 16; case 997: return 17; case 998: return 18;
            case 999: return 19; case 1000: return 20; case 1001: return 21;
            case 1002: return 22; case 1003: return 23; case 1004: return 24;
            default:  return 0;
        }
    }

    // Resolve Pawn → (body_asm_id, equip_point) → vector slot.
    static std::vector<LazyFName>* ResolveCycle(ATgPawn* Pawn, int equipPoint) {
        if (!Pawn || equipPoint <= 0) return nullptr;
        auto it = TgGame__SpawnBotById::m_spawnedBotIds.find(reinterpret_cast<int>(Pawn));
        if (it == TgGame__SpawnBotById::m_spawnedBotIds.end()) return nullptr;
        int asmId = LookupBodyAsmId(it->second);
        if (asmId == 0) return nullptr;
        auto cit = g_OriginCycle.find(MakeKey(asmId, equipPoint));
        if (cit == g_OriginCycle.end() || cit->second.empty()) return nullptr;
        return &cit->second;
    }

}  // namespace

void SocketCycle::Init() {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        Logger::Log("default", "[SocketCycle] no DB connection, skipping\n");
        return;
    }

    // Join mesh-FX rows to the resource name table so we get the actual
    // socket name string out the back. Filter to display_order > 0 (the
    // cycle steps; order 0 entries are always-on FX, not cycle slots),
    // socket_res_id > 0 (zero means "no socket — emit at pawn body"), and
    // resource name containing "Origin" (we want projectile-origin sockets,
    // not muzzle-flash "Emit" sockets — those two are separate entries on
    // the same display_order, e.g. WSO_Emit_03 + WSO_Origin_01 on order 1).
    //
    // DISTINCT because the same (asm, slot, order, socket) tuple can appear
    // multiple times in asm_data_set_asm_mesh_fxs when several FX share a
    // socket.
    const char* kSql =
        "SELECT DISTINCT m.asm_id, m.slot_value_id, m.display_order, r.name "
        "FROM asm_data_set_asm_mesh_fxs m "
        "JOIN asm_data_set_resources r ON m.socket_res_id = r.res_id "
        "WHERE m.display_order > 0 AND m.socket_res_id > 0 "
        "AND r.name LIKE '%Origin%' "
        "ORDER BY m.asm_id, m.slot_value_id, m.display_order;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Log("default", "[SocketCycle] prepare failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    int rowCount = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int asmId       = sqlite3_column_int(stmt, 0);
        int slotValueId = sqlite3_column_int(stmt, 1);
        int order       = sqlite3_column_int(stmt, 2);
        const unsigned char* name = sqlite3_column_text(stmt, 3);
        if (!name || order <= 0) continue;

        int equipPoint = EquipPointFromSlotValueId(slotValueId);
        if (equipPoint == 0) continue;  // unmapped slot — skip rather than poison key 0

        const uint64_t key = MakeKey(asmId, equipPoint);
        auto& vec = g_OriginCycle[key];

        // display_order is 1-indexed and dense per row; grow vector as needed.
        // Just store the raw string — FName conversion is deferred to runtime
        // (see header comment for why we don't touch GNames at init time).
        if ((int)vec.size() < order) vec.resize(order);
        vec[order - 1].src = reinterpret_cast<const char*>(name);

        rowCount++;
    }
    sqlite3_finalize(stmt);

    Logger::Log("default", "[SocketCycle] loaded %d origin-socket rows into %zu (asm, slot) cycles\n",
                rowCount, g_OriginCycle.size());
}

int SocketCycle::LookupOriginSocketCount(ATgPawn* Pawn, int equipPoint) {
    auto* vec = ResolveCycle(Pawn, equipPoint);
    return vec ? (int)vec->size() : 0;
}

int SocketCycle::GetBodyAsmId(ATgPawn* Pawn) {
    if (!Pawn) return 0;
    auto it = TgGame__SpawnBotById::m_spawnedBotIds.find(reinterpret_cast<int>(Pawn));
    if (it == TgGame__SpawnBotById::m_spawnedBotIds.end()) return 0;
    return LookupBodyAsmId(it->second);
}

FName SocketCycle::GetOriginSocketName(ATgPawn* Pawn, int equipPoint, int index) {
    auto* vec = ResolveCycle(Pawn, equipPoint);
    if (!vec || index < 0 || index >= (int)vec->size()) return FName(0);

    LazyFName& slot = (*vec)[index];
    if (!slot.resolved) {
        // First runtime access — engine GNames is hot, safe to construct.
        // Subsequent calls hit the FName NameCache fast-path.
        slot.cached   = FName(const_cast<char*>(slot.src.c_str()));
        slot.resolved = true;
    }
    return slot.cached;
}
