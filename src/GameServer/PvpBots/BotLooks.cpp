#include "src/pch.hpp"
#include "src/GameServer/PvpBots/BotLooks.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <map>
#include <random>
#include <set>
#include "lib/nlohmann/json.hpp"
#include "src/Database/Database.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Core/FMallocWindows/GAllocator.hpp"
#include "src/GameServer/Cosmetics/CosmeticEquip.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace BotLooks {

namespace {

struct SuitSet { int suit; int helmet; };  // helmet 0 = none

struct Pools {
    bool loaded = false;
    std::map<std::string, std::vector<SuitSet>> common;  // class -> matched sets
    std::map<std::string, std::vector<int>> rare;        // class -> suits (bare head)
    std::vector<int> hairM, hairF, dyeCommon, dyeRare, trails;
    std::vector<std::wstring> names;
    // weights (percent)
    int femalePct = 30, rarePct = 25, helmetOffPct = 20, dyePct = 60,
        rareDyePct = 25, secondDyePct = 50, trailPct = 30;
    int headM = 1605, headF = 1593;  // builder MorphBaseHead (595) / female (851)
};

Pools g_pools;
std::map<int, Look> g_looks;
std::set<std::wstring> g_namesInUse;
int g_tempKey = 0;
std::mt19937 g_rng;

const char* kNames[] = {
    "Vance", "Mercer", "Shade", "Bolt", "Kestrel", "Ridge", "Talon", "Juno",
    "Harlow", "Vex", "Orrin", "Sable", "Rook", "Ember", "Quill", "Dax",
    "Marlow", "Pike", "Nyx", "Corvin", "Halcyon", "Brisk", "Tamsin", "Gage",
    "Lyric", "Stroud", "Wren", "Ajax", "Calder", "Mira", "Fenwick", "Rhett",
    "Sloane", "Kaito", "Odessa", "Brant", "Zephyr", "Cobalt", "Ines", "Thorne",
    "Lark", "Radek", "Selene", "Hollis", "Voss", "Anvil", "Petra", "Jace",
    "Torin", "Kira", "Mace", "Delphi", "Strick", "Nova", "Garrick", "Ysolde",
    "Brick", "Faye", "Quorra", "Dorian", "Hatch", "Rilla", "Sorrel", "Kane",
};

std::vector<int> QueryInts(sqlite3* db, const char* sql) {
    std::vector<int> out;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(s) == SQLITE_ROW) out.push_back(sqlite3_column_int(s, 0));
    sqlite3_finalize(s);
    return out;
}

// Item has a mesh row for both genders (else the female/male body shows the
// class default instead of the set).
const char* kBothGenders =
    "(SELECT item_id FROM asm_data_set_item_mesh_asm_groups GROUP BY item_id "
    "HAVING COUNT(DISTINCT gender_type_value_id) >= 2)";

void LoadClass(sqlite3* db, const std::string& cls, const std::string& Cls) {
    // Matched sets: "Suit - <Class> <Set>" + "Helm - <Class> <Set>".
    std::string sql =
        "WITH nm AS (SELECT i.item_id id, m.message n, i.item_subtype_value_id st "
        "FROM asm_data_set_items i JOIN asm_data_set_msg_translations m ON m.msg_id = i.name_msg_id "
        "WHERE i.item_type_value_id = 950 AND i.item_subtype_value_id IN (1006, 1008) "
        "AND i.item_id IN " + std::string(kBothGenders) + ") "
        "SELECT MIN(s.id), MIN(h.id) FROM nm s JOIN nm h ON s.st = 1008 AND h.st = 1006 "
        "AND s.n LIKE 'Suit - " + Cls + " %' AND h.n = 'Helm - ' || substr(s.n, 8) "
        "GROUP BY s.n";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) == SQLITE_OK) {
        while (sqlite3_step(st) == SQLITE_ROW)
            g_pools.common[cls].push_back({sqlite3_column_int(st, 0), sqlite3_column_int(st, 1)});
        sqlite3_finalize(st);
    }
    // Rarer suits: class HoloSuits + Commonwealth suits (Founders/Veteran too).
    sql = "SELECT i.item_id FROM asm_data_set_items i JOIN asm_data_set_msg_translations m "
          "ON m.msg_id = i.name_msg_id WHERE i.item_type_value_id = 950 AND i.item_subtype_value_id = 1008 "
          "AND (m.message LIKE 'HoloSuit - " + Cls + " %' OR m.message LIKE 'Suit - " + Cls + " Commonwealth%') "
          "AND i.item_id IN " + std::string(kBothGenders) + " ORDER BY i.item_id";
    g_pools.rare[cls] = QueryInts(db, sql.c_str());
}

void LoadConfig() {
    FILE* f = fopen("pvp-bot-looks.json", "rb");
    if (!f) return;
    std::string txt;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) txt.append(buf, n);
    fclose(f);
    nlohmann::json j = nlohmann::json::parse(txt, nullptr, false);
    if (!j.is_object()) {
        Logger::Log("chat-command", "[BotLooks] pvp-bot-looks.json is not valid JSON - ignored\n");
        return;
    }
    auto pct = [&](const char* k, int& v) {
        if (j.contains(k) && j[k].is_number_integer()) v = std::clamp(j[k].get<int>(), 0, 100);
    };
    pct("female_pct", g_pools.femalePct);
    pct("rare_suit_pct", g_pools.rarePct);
    pct("helmet_off_pct", g_pools.helmetOffPct);
    pct("dye_pct", g_pools.dyePct);
    pct("rare_dye_pct", g_pools.rareDyePct);
    pct("second_dye_pct", g_pools.secondDyePct);
    pct("trail_pct", g_pools.trailPct);
    if (j.contains("names") && j["names"].is_array()) {
        std::vector<std::wstring> names;
        for (auto& e : j["names"]) {
            if (!e.is_string()) continue;
            const std::string s = e.get<std::string>();
            if (s.empty() || s.size() > 20) continue;
            std::wstring w;
            for (unsigned char c : s) if (c >= 32 && c < 127) w.push_back((wchar_t)c);
            if (!w.empty()) names.push_back(w);
        }
        if (names.size() >= 8) g_pools.names = names;
    }
    Logger::Log("chat-command", "[BotLooks] config loaded (female %d%% rare %d%% names %d)\n",
        g_pools.femalePct, g_pools.rarePct, (int)g_pools.names.size());
}

void Load() {
    if (g_pools.loaded) return;
    g_pools.loaded = true;
    g_rng.seed((unsigned)time(nullptr) ^ ((unsigned)GetCurrentProcessId() << 16));
    for (const char* nm : kNames) {
        std::wstring w;
        for (const char* p = nm; *p; ++p) w.push_back((wchar_t)*p);
        g_pools.names.push_back(w);
    }
    if (sqlite3* db = Database::GetConnection()) {
        LoadClass(db, "assault", "Assault");
        LoadClass(db, "medic", "Medic");
        LoadClass(db, "recon", "Recon");
        LoadClass(db, "robotics", "Robotics");
        // Builder hair: 596 = male rows, 850 = female rows (asm 403 = bald, skipped).
        g_pools.hairM = QueryInts(db, "SELECT asm_id FROM asm_data_set_assembly_meshes "
            "WHERE asm_mesh_type_value_id = 596 AND asm_id <> 403 AND name LIKE 'NewHair%'");
        g_pools.hairF = QueryInts(db, "SELECT asm_id FROM asm_data_set_assembly_meshes "
            "WHERE asm_mesh_type_value_id = 850 AND name LIKE 'NewHair%'");
        g_pools.dyeCommon = QueryInts(db, "SELECT i.item_id FROM asm_data_set_items i JOIN asm_data_set_msg_translations m "
            "ON m.msg_id = i.name_msg_id WHERE i.item_type_value_id = 1020 "
            "AND (m.message LIKE 'Dye Tier1 -%' OR m.message LIKE 'Dye Tier2 -%' OR m.message LIKE 'Dye Tier3 -%')");
        g_pools.dyeRare = QueryInts(db, "SELECT i.item_id FROM asm_data_set_items i JOIN asm_data_set_msg_translations m "
            "ON m.msg_id = i.name_msg_id WHERE i.item_type_value_id = 1020 "
            "AND (m.message LIKE 'Dye Tier4 -%' OR m.message LIKE 'Dye Tier5 -%')");
        g_pools.trails = QueryInts(db, "SELECT i.item_id FROM asm_data_set_items i JOIN asm_data_set_msg_translations m "
            "ON m.msg_id = i.name_msg_id WHERE i.item_type_value_id = 1612 "
            "AND m.message LIKE 'Trail - %' AND m.message NOT LIKE '%(Test)%'");
    }
    LoadConfig();
    Logger::Log("chat-command",
        "[BotLooks] pools: sets a=%d m=%d r=%d ro=%d rare a=%d m=%d r=%d ro=%d hairM=%d hairF=%d dyes=%d+%d trails=%d\n",
        (int)g_pools.common["assault"].size(), (int)g_pools.common["medic"].size(),
        (int)g_pools.common["recon"].size(), (int)g_pools.common["robotics"].size(),
        (int)g_pools.rare["assault"].size(), (int)g_pools.rare["medic"].size(),
        (int)g_pools.rare["recon"].size(), (int)g_pools.rare["robotics"].size(),
        (int)g_pools.hairM.size(), (int)g_pools.hairF.size(),
        (int)g_pools.dyeCommon.size(), (int)g_pools.dyeRare.size(), (int)g_pools.trails.size());
}

bool Roll(int pct) { return (int)(g_rng() % 100) < pct; }

template <class T> const T* PickFrom(const std::vector<T>& v) {
    return v.empty() ? nullptr : &v[g_rng() % v.size()];
}

std::wstring PickName() {
    std::vector<const std::wstring*> free;
    for (const auto& n : g_pools.names) if (!g_namesInUse.count(n)) free.push_back(&n);
    std::wstring base;
    if (!free.empty()) base = *free[g_rng() % free.size()];
    else base = g_pools.names[g_rng() % g_pools.names.size()] + std::to_wstring(2 + g_rng() % 98);
    g_namesInUse.insert(base);
    return base;
}

Look MakeLook(const std::string& cls) {
    Load();
    Look l;
    const bool female = Roll(g_pools.femalePct) && !g_pools.hairF.empty();
    l.gender = female ? GA_G::GENDER_TYPE_ID_FEMALE : GA_G::GENDER_TYPE_ID_MALE;
    l.head = female ? g_pools.headF : g_pools.headM;
    if (const int* h = PickFrom(female ? g_pools.hairF : g_pools.hairM)) l.hair = *h;
    const auto& sets = g_pools.common[cls];
    const auto& rare = g_pools.rare[cls];
    if (!rare.empty() && (sets.empty() || Roll(g_pools.rarePct))) {
        l.suit = *PickFrom(rare);
    } else if (const SuitSet* s = PickFrom(sets)) {
        l.suit = s->suit;
        l.helmet = Roll(g_pools.helmetOffPct) ? 0 : s->helmet;
    }
    if (Roll(g_pools.dyePct)) {
        const auto& pool = (Roll(g_pools.rareDyePct) && !g_pools.dyeRare.empty()) ? g_pools.dyeRare : g_pools.dyeCommon;
        if (const int* d = PickFrom(pool)) l.dye1 = *d;
        if (l.dye1 && Roll(g_pools.secondDyePct))
            if (const int* d2 = PickFrom(g_pools.dyeCommon)) l.dye2 = *d2;
    }
    if (Roll(g_pools.trailPct))
        if (const int* t = PickFrom(g_pools.trails)) l.trail = *t;
    l.name = L"[BOT] " + PickName();
    return l;
}

void SetName(ATgPawn* pawn, const std::wstring& name) {
    if (!pawn->PlayerReplicationInfo) return;
    FString s;
    const int len = (int)name.size() + 1;
    s.Data = (wchar_t*)GAllocator::Malloc(len * sizeof(wchar_t));
    s.Count = s.Max = len;
    wcscpy(s.Data, name.c_str());
    pawn->PlayerReplicationInfo->eventSetPlayerName(s);
    GAllocator::Free(s.Data);
    s.Data = nullptr; s.Count = s.Max = 0;
}

}  // namespace

const Look& ForKey(int key, const std::string& cls) {
    static int s_nextTemp = 0;
    if (key == 0) key = g_tempKey = --s_nextTemp;
    auto it = g_looks.find(key);
    if (it == g_looks.end()) it = g_looks.emplace(key, MakeLook(cls)).first;
    return it->second;
}

int LastTempKey() { return g_tempKey; }

void Rekey(int fromKey, int toKey) {
    auto it = g_looks.find(fromKey);
    if (it == g_looks.end() || fromKey == toKey) return;
    g_looks[toKey] = it->second;
    g_looks.erase(it);
}

void Release(int key) {
    auto it = g_looks.find(key);
    if (it == g_looks.end()) return;
    g_namesInUse.erase(it->second.name.substr(6));  // strip "[BOT] "
    g_looks.erase(it);
}

bool Apply(ATgPawn* pawn, const Look& l) {
    if (!pawn) return false;
    CosmeticEquip::SetBotBody(pawn, l.gender, l.head, l.hair);
    const bool suit = l.suit && CosmeticEquip::ApplyItemToBot(pawn, l.suit);
    if (suit) {
        if (l.helmet) CosmeticEquip::ApplyItemToBot(pawn, l.helmet);
        if (l.dye1) CosmeticEquip::ApplyItemToBotSlot(pawn, l.dye1, 16);
        if (l.dye2) CosmeticEquip::ApplyItemToBotSlot(pawn, l.dye2, 17);
        if (l.trail) CosmeticEquip::ApplyItemToBot(pawn, l.trail);
    }
    SetName(pawn, l.name);
    Logger::Log("chat-command",
        "[BotLooks] pawn=%d gender=%d head=%d hair=%d suit=%d(%d) helm=%d dye=%d/%d trail=%d\n",
        (int)pawn->r_nPawnId, l.gender, l.head, l.hair, l.suit, (int)suit, l.helmet, l.dye1, l.dye2, l.trail);
    return suit;
}

}  // namespace BotLooks
