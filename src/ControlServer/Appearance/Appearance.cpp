#include "src/ControlServer/Appearance/Appearance.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/EconParity/EconParity.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <cstring>
#include <map>

namespace Appearance {
namespace {
constexpr uint32_t kCyberCut = 6019, kGenolab = 6128;
constexpr int kCredits = 1603, kTokens = 1604;

// The shop row must exist and the price is the game's own (lowest id per
// currency, as the vendor list sends it); credits/tokens only (no AP).
int ShopPrice(sqlite3* db, const Request& q, int* currency) {
    sqlite3_stmt* s = nullptr; int price = 0; *currency = 0;
    if (sqlite3_prepare_v2(db,
            "SELECT p.price, p.currency_type_value_id FROM asm_data_set_loot_table_items li "
            "JOIN asm_data_set_prices p ON p.item_id = li.item_id AND p.price > 0 "
            " AND p.id = (SELECT MIN(p2.id) FROM asm_data_set_prices p2 WHERE p2.item_id = p.item_id "
            "             AND p2.currency_type_value_id = p.currency_type_value_id) "
            "WHERE li.loot_table_id = ? AND li.loot_table_item_id = ? AND li.item_id = ? "
            "  AND p.currency_type_value_id IN (1603, 1604) "
            "ORDER BY (p.currency_type_value_id = ?) DESC, p.currency_type_value_id DESC LIMIT 1", -1, &s, nullptr) != SQLITE_OK)
        return 0;
    sqlite3_bind_int(s, 1, (int)q.loot_table_id); sqlite3_bind_int(s, 2, (int)q.loot_table_item_id);
    sqlite3_bind_int(s, 3, (int)q.item_id); sqlite3_bind_int(s, 4, (int)q.currency);
    if (sqlite3_step(s) == SQLITE_ROW) { price = sqlite3_column_int(s, 0); *currency = sqlite3_column_int(s, 1); }
    sqlite3_finalize(s);
    return price;
}

std::map<uint32_t, uint32_t> Pairs(const void* p, size_t n) {
    std::map<uint32_t, uint32_t> m;
    const uint8_t* b = (const uint8_t*)p;
    for (size_t i = 0; i + 8 <= n; i += 8) {
        uint32_t k, v; std::memcpy(&k, b + i, 4); std::memcpy(&v, b + i + 4, 4);
        m[k] = v;
    }
    return m;
}
}  // namespace

Result Apply(int64_t character_id, const Request& q) {
    Result r;
    sqlite3* db = Database::GetConnection();
    if (!db || character_id <= 0) { r.reason = "no agent"; return r; }
    if (q.item_id != kCyberCut && q.item_id != kGenolab) { r.reason = "not an appearance shop item"; }
    else if (q.morph.size() % 8 != 0 || q.morph.size() > 8 * 512) { r.reason = "bad appearance data"; }
    else if ((r.price = ShopPrice(db, q, &r.currency)) <= 0) { r.reason = "not sold here"; }
    if (!r.reason.empty()) {
        Logger::Log("tcp", "[Appearance] char=%lld item=%u table=%u row=%u -> REFUSED (%s)\n",
            (long long)character_id, q.item_id, q.loot_table_id, q.loot_table_item_id, r.reason.c_str());
        return r;
    }

    // current look
    uint32_t head = 0, hair = 0; std::vector<uint8_t> blob; sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT head_asm_id, hair_asm_id, morph_data FROM ga_characters WHERE id = ? AND deleted_at = 0",
                           -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, character_id);
        if (sqlite3_step(s) == SQLITE_ROW) {
            head = (uint32_t)sqlite3_column_int(s, 0); hair = (uint32_t)sqlite3_column_int(s, 1);
            const void* b = sqlite3_column_blob(s, 2); const int n = sqlite3_column_bytes(s, 2);
            if (b && n > 0) blob.assign((const uint8_t*)b, (const uint8_t*)b + n);
            r.ok = true;
        }
        sqlite3_finalize(s);
    }
    if (!r.ok) { r.reason = "no agent"; return r; }
    r.ok = false;

    auto m = Pairs(blob.data(), blob.size());
    for (const auto& [k, v] : Pairs(q.morph.data(), q.morph.size())) m[k] = v;
    std::vector<uint8_t> out; out.reserve(m.size() * 8);
    for (const auto& [k, v] : m) { uint8_t t[8]; std::memcpy(t, &k, 4); std::memcpy(t + 4, &v, 4); out.insert(out.end(), t, t + 8); }
    uint32_t newHead = q.head_asm_id ? q.head_asm_id : head, newHair = q.item_id == kCyberCut && q.hair_asm_id ? q.hair_asm_id : hair;
    EconParity::SanitizeCreateLooks(newHead, newHair);

    const char* col = r.currency == kTokens ? "tokens" : "credits";
    const std::string pay = std::string("UPDATE ga_characters SET ") + col + " = " + col + " - ?1 WHERE id = ?2 AND deleted_at = 0 AND " + col + " >= ?1";
    sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
    bool ok = false;
    if (sqlite3_prepare_v2(db, pay.c_str(), -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, r.price); sqlite3_bind_int64(s, 2, character_id);
        ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
        sqlite3_finalize(s);
    }
    if (!ok) r.reason = r.currency == kTokens ? "not enough tokens" : "not enough credits";
    if (ok && sqlite3_prepare_v2(db, "UPDATE ga_characters SET head_asm_id = ?, hair_asm_id = ?, morph_data = ? WHERE id = ?",
                                 -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, (int)newHead); sqlite3_bind_int(s, 2, (int)newHair);
        sqlite3_bind_blob(s, 3, out.data(), (int)out.size(), SQLITE_TRANSIENT); sqlite3_bind_int64(s, 4, character_id);
        ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
        sqlite3_finalize(s);
        if (!ok) r.reason = "db write failed";
    }
    sqlite3_exec(db, ok ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
    r.ok = ok;
    Logger::Log("tcp", "[Appearance] char=%lld item=%u (%s) %d %s head %u->%u hair %u->%u morph pairs %zu->%zu (sent %zu) -> %s %s\n",
        (long long)character_id, q.item_id, q.item_id == kCyberCut ? "Cyber Cuts" : "Genolab", r.price, col,
        head, newHead, hair, newHair, blob.size() / 8, m.size(), q.morph.size() / 8, ok ? "OK" : "REFUSED", r.reason.c_str());
    return r;
}

}  // namespace Appearance
