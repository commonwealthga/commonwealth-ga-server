#include "src/ControlServer/Loadouts/ModResolver.hpp"

#include <map>
#include <mutex>
#include <string>

#include "sqlite3.h"

#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"

namespace {

// device_id → letter → quality_value_id → egid (innate, from this device's pool)
std::map<int, std::map<char, std::map<int, int>>> g_innateTable;

// device_id → letter → prop_id (the prop this letter targets on this device,
// derived from any blueprint of this device — first-seen wins)
std::map<int, std::map<char, int>> g_letterToProp;

std::once_flag g_initOnce;

// Kit egid table by prop_id. Per-quality columns: COMMON / UNCOMMON / RARE / EPIC.
// 0 = no kit shipped at that tier (drops the letter at Resync time).
struct KitEgids { int common, uncommon, rare, epic; };

const std::map<int, KitEgids> kPropToKit = {
    // 'd' damage props — all share the Effect Damage Mod kit family except
    // Pet Damage which has its own. The visible letter is 'd' regardless.
    { 65,  { 0, 24191, 24195, 24199 } },  // Effect Damage Modifier
    { 212, { 0, 24191, 24195, 24199 } },  // Damage - Melee     (reuse Effect Damage kit)
    { 214, { 0, 24191, 24195, 24199 } },  // Damage - Range     (reuse Effect Damage kit)
    { 350, { 0, 25321, 25322, 25323 } },  // Pet Damage Modifier

    { 330, { 0, 24208, 24211, 24212 } },  // 'h' Effect Healing Modifier
    { 203, { 0, 24188, 24200, 24201 } },  // 'c' Recharge Time Modifier
    { 242, { 0, 24233, 24234, 24230 } },  // 'p' Power Pool Cost

    // 'r' family — three different props all share the Armor Ballistics kit
    // (only kit egid family with ui_code 'r' that ships).
    { 114, { 0, 24144, 24163, 24165 } },  // Device Range Modifier
    { 218, { 0, 24144, 24163, 24165 } },  // Protection - Ranged
    { 381, { 0, 24144, 24163, 24165 } },  // Pet Range Modifier

    { 219, { 0, 24081, 23948, 24083 } },  // 'b' Protection - AOE (Armor Flak)
    { 412, { 0, 24072, 24073, 24074 } },  // 'n' Health Max Modifier (Armor Padding)
    { 217, { 0, 24141, 24168, 24170 } },  // 'm' Protection - Melee  (Armor Plate)
    { 421, { 0, 26088, 26089, 26090 } },  // 'T' Threat Modifier

    // Single-tier kit egids — same egid at every quality, kept for
    // letter resolution coverage. The shipped DB has only one canonical
    // egid for each of these props with a ui_code.
    { 352, { 16255, 16255, 16255, 16255 } },  // 'x' AOE Radius Modifier
    { 208, { 24420, 24420, 24420, 24420 } },  // 't' Effect Lifetime Modifier
    { 386, { 24503, 24503, 24503, 24503 } },  // 's' Effect Shield Modifier
    { 210, { 27742, 27742, 27742, 27742 } },  // 'y' Effect Heal (Self)
    { 207, { 13381, 13381, 13381, 13381 } },  // 'q' Device Effective Range
    { 355, { 24601, 24601, 24601, 24601 } },  // 'l' Pet LifeSpan
    { 155, { 16601, 16601, 16601, 16601 } },  // 'g' Protection - Physical
    { 113, { 22312, 22312, 22312, 22312 } },  // 'a' Accuracy
    { 137, {  7312,  7312,  7312,  7312 } },  // 'f' Falling Damage

    // Note: prop 339 'n' (Health Max Deployables) and prop 366 'v' (Pet Max
    // Health) intentionally absent — every shipped kit producing 'v' or
    // deployable-'n' is the multi-letter Survivor kit (handled separately,
    // fired only for 'vn' / 'nv' pairs).
};

// Survivor (multi-letter) kit — emits 'v' AND 'n' in the suffix from one egid.
constexpr KitEgids kSurvivorKit = { 0, 24222, 24223, 24219 };

int PickByQuality(const KitEgids& k, int q) {
    switch (q) {
        case 1165: return k.common;    // Q_COMMON
        case 1164: return k.uncommon;  // Q_UNCOMMON
        case 1163: return k.rare;      // Q_RARE
        case 1162: return k.epic;      // Q_EPIC
        default:   return 0;
    }
}

int KitEgidForProp(int prop_id, int quality) {
    auto it = kPropToKit.find(prop_id);
    if (it == kPropToKit.end()) return 0;
    return PickByQuality(it->second, quality);
}

int SurvivorKitEgid(int quality) {
    return PickByQuality(kSurvivorKit, quality);
}

void DoInit() {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        Logger::Log("db", "[ModResolver] Init: no DB connection — resolver will return empty\n");
        return;
    }

    // Walk every blueprint tied to a created_item_id, collecting
    // (device, letter, quality, egid, prop). Pick MIN(egid) per
    // (device, letter, quality) for stable choices across runs.
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT b.created_item_id, p.ui_code, bim.quality_value_id, "
        "       bmg.effect_group_id, e.prop_id "
        "FROM asm_data_set_blueprints b "
        "JOIN asm_data_set_blueprint_item_mods bim "
        "  ON bim.blueprint_id = b.blueprint_id "
        "JOIN asm_data_set_blueprint_mod_effect_groups bmg "
        "  ON bmg.blueprint_mod_id = bim.blueprint_mod_id "
        "JOIN asm_data_set_effects e "
        "  ON e.effect_group_id = bmg.effect_group_id "
        "JOIN asm_data_set_properties p "
        "  ON p.prop_id = e.prop_id "
        "WHERE p.ui_code != ''";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Log("db", "[ModResolver] Init: prepare failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    int rows = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int device_id   = sqlite3_column_int(stmt, 0);
        const char* code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (!code || !code[0]) continue;
        char letter = code[0];           // ui_code is single-character in practice
        int quality = sqlite3_column_int(stmt, 2);
        int egid    = sqlite3_column_int(stmt, 3);
        int prop_id = sqlite3_column_int(stmt, 4);

        int& egidSlot = g_innateTable[device_id][letter][quality];
        if (egidSlot == 0 || egid < egidSlot) egidSlot = egid;

        int& propSlot = g_letterToProp[device_id][letter];
        if (propSlot == 0) propSlot = prop_id;

        ++rows;
    }
    sqlite3_finalize(stmt);

    Logger::Log("db", "[ModResolver] Init: indexed %d rows across %d devices\n",
        rows, (int)g_innateTable.size());
}

int LookupInnate(int device_id, char letter, int quality) {
    auto di = g_innateTable.find(device_id);
    if (di == g_innateTable.end()) return 0;
    auto li = di->second.find(letter);
    if (li == di->second.end()) return 0;
    auto qi = li->second.find(quality);
    if (qi == li->second.end()) return 0;
    return qi->second;
}

int LookupLetterProp(int device_id, char letter) {
    auto di = g_letterToProp.find(device_id);
    if (di == g_letterToProp.end()) return 0;
    auto li = di->second.find(letter);
    if (li == di->second.end()) return 0;
    return li->second;
}

// Look up the Output Mod egid (prop 385, Common-tier slot of the device's
// blueprint). Standard items ship with +70%, OC items with +75%, a few
// outliers with other values. Cached per (device_id, oc).
int LookupOutputModEgid(int device_id, bool oc) {
    static std::map<std::pair<int, bool>, int> cache;
    const auto key = std::make_pair(device_id, oc);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;

    sqlite3* db = Database::GetConnection();
    if (!db) { cache[key] = 0; return 0; }

    // Output Mod lives on the Common-tier slot (quality_value_id=1165) of the
    // blueprint's mod table — one egid per blueprint, prop 385.
    const char* sql = oc
        ? "SELECT bmg.effect_group_id FROM asm_data_set_blueprints b "
          "  JOIN asm_data_set_blueprint_item_mods bim ON bim.blueprint_id=b.blueprint_id "
          "  JOIN asm_data_set_blueprint_mod_effect_groups bmg ON bmg.blueprint_mod_id=bim.blueprint_mod_id "
          "  JOIN asm_data_set_effects e ON e.effect_group_id=bmg.effect_group_id "
          " WHERE e.prop_id=385 AND b.created_item_id=? AND b.override_name_msg_id != 0 "
          " ORDER BY b.blueprint_id LIMIT 1"
        : "SELECT bmg.effect_group_id FROM asm_data_set_blueprints b "
          "  JOIN asm_data_set_blueprint_item_mods bim ON bim.blueprint_id=b.blueprint_id "
          "  JOIN asm_data_set_blueprint_mod_effect_groups bmg ON bmg.blueprint_mod_id=bim.blueprint_mod_id "
          "  JOIN asm_data_set_effects e ON e.effect_group_id=bmg.effect_group_id "
          " WHERE e.prop_id=385 AND b.created_item_id=? AND b.override_name_msg_id=0 "
          " ORDER BY b.blueprint_id LIMIT 1";

    int egid = 0;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, device_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            egid = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    cache[key] = egid;
    return egid;
}

}  // namespace

namespace ModResolver {

std::vector<int> Resolve(int device_id, int quality, const Mods::Result& mods) {
    std::call_once(g_initOnce, DoInit);

    // Back-compat: explicit egid list bypasses letter resolution entirely
    if (!mods.raw_egids.empty()) {
        return mods.raw_egids;
    }

    std::vector<int> out;

    // Output Mod (prop 385): every shipped device's blueprint carries a
    // Common-tier prop-385 egid (+70% standard, +75% OC, a few outliers).
    // Now safe to prepend because rolled-mod buff entries are scoped to
    // the equipping device's `r_nDeviceInstanceId` (see
    // `Inventory::ApplyRolledModEffects`), so 9 devices' Output Mods
    // each land in their own scoped FBuffInfo entry instead of collapsing
    // into one shared 700% slot. Each device's fire-mode reads pick up
    // ONLY its own scoped entry + skill wildcards.
    int outputEgid = LookupOutputModEgid(device_id, mods.oc);
    if (outputEgid) out.push_back(outputEgid);

    // Innate letters: device-specific lookup, skip unknowns silently
    for (char c : mods.innate) {
        int egid = LookupInnate(device_id, c, quality);
        if (egid) out.push_back(egid);
    }

    // Kit letters: scan with one-character lookahead to detect Survivor pair
    const std::string& kit = mods.kit;
    for (size_t i = 0; i < kit.size();) {
        char c = kit[i];
        char next = (i + 1 < kit.size()) ? kit[i + 1] : 0;

        if ((c == 'v' && next == 'n') || (c == 'n' && next == 'v')) {
            int egid = SurvivorKitEgid(quality);
            if (egid) out.push_back(egid);
            i += 2;
            continue;
        }

        // Lone 'v' has no shipped single-letter kit — drop.
        if (c == 'v') { ++i; continue; }

        // Single-letter kit: derive prop from device's pool, look up kit egid
        int prop_id = LookupLetterProp(device_id, c);
        if (prop_id) {
            int egid = KitEgidForProp(prop_id, quality);
            if (egid) out.push_back(egid);
        }
        ++i;
    }

    return out;
}

}  // namespace ModResolver
