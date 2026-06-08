// Logger channel: "armor"
#include "src/GameServer/Armor/Armor.hpp"
#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

// What we applied for a given armor piece this RCST cycle. Tracked per-pawn
// so Revert can mirror with the exact inverse next pass.
//
// classResId selects the apply/revert path:
//   * 157 (TgEffectBuff) — buff registry via ApplyBuff. Only path that hits
//     ConvertPropToPropList(ITEM,304)={412,390} and refreshes
//     r_nHealthMaximum via RecomputeEagerBaseProp (used by Padding 412 and
//     the baseline +10% Health Mod 390 entry).
//   * 80 (TgEffect) / other — direct prop->m_fRaw write only, no engine
//     fan-out. Required for Protection-Melee/Ranged/AOE (217/218/219)
//     because CalcProtection (TgEffectGroup.uc:745) reads prop.m_fRaw
//     directly. Matches the TgDeployable equip-effect pattern in
//     TgDeployable__InitializeDefaultProps.cpp:200 — same shape, known good.
//
// Multiple AppliedEntry rows per piece because:
//   - 1 base +10% Health Mod (slot's blueprint baseline egid)
//   - up to 6 mod entries (one per egid in the mod CSV; duplicate egids
//     produce duplicate rows — exactly what we want for stacking,
//     mirroring how Inventory::ApplyRolledModEffects works)
struct AppliedEntry {
    int   propId;
    int   calcMethod;
    float amount;
    int   deviceInstId;  // = the armor's ga_players_inventory.id (used as buff devInst)
    int   classResId;    // 157 → ApplyBuff; else → direct m_fRaw write
};

// Per-pawn list of every ApplyBuff call we made during the most recent
// ApplyDefaultArmor pass. Revert iterates this in reverse and ApplyBuffs
// each one with bRemove=1.
//
// Keyed by r_nPawnId, NOT the raw pawn pointer. UE3 reuses freed actor
// addresses, so a pointer key collides a freshly-spawned pawn with a dead
// pawn's stale entry: RevertDefaultArmor (which runs FIRST in RCST, before
// Apply) would then reverse a previous pawn-life's armor deltas against the
// new pawn's base protection props (217/218/219, direct m_fRaw writes),
// driving them below base and zeroing protection — players took ~10-35% more
// damage with the exact magnitude depending on whichever stale entry the
// reused address happened to carry. r_nPawnId is a monotonic per-spawn id
// assigned by TgGame.GetNextPawnId (UC TgPawn.PostBeginPlay), unique within
// an instance and never reused, so a dead pawn's entry can never be found by
// a live pawn.
std::map<int, std::vector<AppliedEntry>>& Records() {
    static std::map<int, std::vector<AppliedEntry>> g;
    return g;
}

// Identify a player pawn without depending on Pawn->Controller being wired.
// On respawn / mission travel the controller is wired asynchronously via
// Possess() AFTER RCST runs; a controller-based check returned false there
// and armor was silently skipped. PRI + r_bIsBot is reliable: PRI is always
// present for bots and players; r_bIsBot is CPF_Net + always set at bot
// spawn.
bool IsHumanPlayer(ATgPawn* Pawn) {
    if (!Pawn) return false;
    if (!Pawn->PlayerReplicationInfo) return false;
    if (Pawn->r_bIsBot) return false;
    return true;
}

// Parse a comma-separated egid list into a vector. Empty/whitespace tokens
// are skipped. Anything non-numeric ends parsing early (treating it as
// corruption rather than silently dropping). Returns the list in input
// order — order matters here, because every egid in the CSV is one stacking
// instance (six identical egids = six ApplyBuff calls).
std::vector<int> ParseEgidCsv(const char* csv) {
    std::vector<int> out;
    if (!csv) return out;
    const char* p = csv;
    while (*p) {
        while (*p == ',' || *p == ' ') ++p;
        if (!*p) break;
        char* end = nullptr;
        long v = std::strtol(p, &end, 10);
        if (end == p) break;
        out.push_back((int)v);
        p = end;
    }
    return out;
}

// Resolve every effect row for the given egids in one DB roundtrip. Returns
// a map from egid → list of (propId, calcMethod, baseValue), filtered to
// the calc methods our ApplyBuff implements (67/68/69/70) and to permanent
// (lifetime_sec=0) effects only.
//
// The lifetime gate is via a subquery (not a JOIN) because
// asm_data_set_effect_groups has duplicate rows per effect_group_id in this
// build's asm.dat capture — a JOIN doubles every effect, inflating buff
// magnitudes 2×. Same shape as Inventory::ApplyRolledModEffects.
struct EffectRow { int propId; float baseValue; int calcMethod; int classResId; };

std::map<int, std::vector<EffectRow>>
LookupEffectRows(const std::vector<int>& egids) {
    std::map<int, std::vector<EffectRow>> out;
    if (egids.empty()) return out;
    sqlite3* db = Database::GetConnection();
    if (!db) return out;

    std::vector<int> uniq = egids;
    std::sort(uniq.begin(), uniq.end());
    uniq.erase(std::unique(uniq.begin(), uniq.end()), uniq.end());

    std::string sql =
        "SELECT e.effect_group_id, e.prop_id, e.base_value, e.calc_method_value_id, e.class_res_id "
        "FROM asm_data_set_effects e "
        "WHERE e.effect_group_id IN (SELECT effect_group_id FROM asm_data_set_effect_groups WHERE lifetime_sec = 0) "
        "  AND e.effect_group_id IN (";
    for (size_t i = 0; i < uniq.size(); ++i) {
        if (i) sql += ',';
        sql += std::to_string(uniq[i]);
    }
    sql += ')';

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Log("armor", "LookupEffectRows: prepare failed: %s\n", sqlite3_errmsg(db));
        return out;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int   egid       = sqlite3_column_int   (stmt, 0);
        const int   propId     = sqlite3_column_int   (stmt, 1);
        const float baseValue  = (float)sqlite3_column_double(stmt, 2);
        const int   calcMethod = sqlite3_column_int   (stmt, 3);
        const int   classResId = sqlite3_column_int   (stmt, 4);
        if (calcMethod < 67 || calcMethod > 70) continue;
        out[egid].push_back({ propId, baseValue, calcMethod, classResId });
    }
    sqlite3_finalize(stmt);
    return out;
}

// Find a property in Pawn->s_Properties by propId. Linear scan — fine, array
// is < 50 entries and runs once per RCST cycle.
UTgProperty* FindProperty(ATgPawn* Pawn, int propId) {
    if (!Pawn || !Pawn->s_Properties.Data) return nullptr;
    for (int i = 0; i < Pawn->s_Properties.Count; ++i) {
        UTgProperty* p = Pawn->s_Properties.Data[i];
        if (p && p->m_nPropertyId == propId) return p;
    }
    return nullptr;
}

// Compute the m_fRaw delta a class-80 (TgEffect) row would apply. Mirrors
// the calc-method semantics in TgDeployable's ApplyDeviceEquipEffects and
// Inventory::ApplyDeviceEquipEffects:
//   67 ADD              +base
//   70 SUBTRACT         -base
//   68 PERC_INCREASE    +propBase*base
//   69 PERC_DECREASE    -propBase*base
// Returns 0 for unsupported calc — caller skips.
float ComputeRawDelta(int calcMethod, float baseValue, float propBase) {
    switch (calcMethod) {
        case 67: return  baseValue;
        case 70: return -baseValue;
        case 68: return  propBase * baseValue;
        case 69: return -propBase * baseValue;
        default: return 0.0f;
    }
}

// Apply one effect row and record what we did. Two-class dispatch:
//   * class 157 (TgEffectBuff) → buff registry via ApplyBuff. Only path
//     that triggers ConvertPropToPropList(ITEM,304)={412,390} expansion +
//     RecomputeEagerBaseProp(304) for r_nHealthMaximum fan-out. Used by
//     Padding (412) and the slot baseline (390).
//   * class 80 (TgEffect) / other → direct prop->m_fRaw write only.
//     Required for Protection-Melee/Ranged/AOE (217/218/219) because
//     CalcProtection (TgEffectGroup.uc:745) reads prop.m_fRaw directly,
//     bypassing the buff registry. NO ApplyProperty call — matches the
//     TgDeployable equip-effect helper (working pattern, see
//     TgDeployable__InitializeDefaultProps.cpp:200 + its comment block
//     about skipping SetProperty fan-out).
void ApplyAndRecord(ATgPawn* Pawn,
                    std::vector<AppliedEntry>& records,
                    int propId, int calcMethod, float amount, int devInst,
                    int classResId) {
    if (classResId == 157) {
        FBuffHeader h{};
        h.nPropId          = propId;
        h.nReqCategoryCode = 0;
        h.nReqSkillId      = 0;
        h.nReqDeviceInstId = devInst;
        TgPawn__ApplyBuff::Call(Pawn, /*edx=*/nullptr, h, calcMethod, amount,
                                /*bRemove=*/0, /*BUFF_SOURCE_TYPE_ITEM=*/1);
    } else {
        UTgProperty* prop = FindProperty(Pawn, propId);
        if (!prop) {
            Logger::Log("armor",
                "[Armor/apply] pawn=%p prop=%d not in s_Properties — skipped (class=%d)\n",
                (void*)Pawn, propId, classResId);
            return;
        }
        const float delta = ComputeRawDelta(calcMethod, amount, prop->m_fBase);
        if (delta == 0.0f) return;
        const float before = prop->m_fRaw;
        prop->m_fRaw = before + delta;
        Logger::Log("armor",
            "[Armor/apply] pawn=%p prop=%d cm=%d base=%.2f class=%d  m_fRaw %.2f -> %.2f\n",
            (void*)Pawn, propId, calcMethod, amount, classResId, before, prop->m_fRaw);
    }
    records.push_back({ propId, calcMethod, amount, devInst, classResId });
}

// What we're about to apply, expressed as one row per equipped armor piece.
struct EquippedPiece {
    int  inventoryId;           // ga_players_inventory.id (used as buff devInst)
    int  equippedSlot;          // group-126 SVID (1107/1120/...) — logging only
    std::string modEgidsCsv;
};

// Walk ga_character_devices joined to ga_players_inventory and return one
// EquippedPiece per equipped armor row for the character. Only rows whose
// equipped_slot matches one of the 7 group-126 armor SVIDs are returned —
// the same query would otherwise pick up cosmetic and gameplay-device rows
// for the same character.
std::vector<EquippedPiece> FetchEquippedArmor(int64_t character_id, int item_profile_id) {
    std::vector<EquippedPiece> out;
    sqlite3* db = Database::GetConnection();
    if (!db) return out;

    // The 7 group-129 armor SVIDs (see EquipSlot.hpp ArmorSlot::All[]).
    // Hardcoding the literal here keeps Armor.cpp from having to drag the
    // EquipSlot.hpp dependency just for one IN-clause; the values are
    // stable (DB-canonical) and small.
    //   1130 Head, 1132 Hands (repurposed), 1133 Chest, 1136 Arms,
    //   1139 Legs, 1142 Feet, 1143 Shoulder (repurposed)
    // See ArmorSlot constants in EquipSlot.hpp for the full decode of
    // MiscItems[] indexing.
    // Profile-scoped: only returns armor rows for the active loadout slot.
    const char* sql =
        "SELECT cd.inventory_id, cd.equipped_slot, pi.mod_effect_group_ids "
        "FROM ga_character_devices cd "
        "JOIN ga_players_inventory pi ON pi.id = cd.inventory_id "
        "WHERE cd.character_id = ? AND cd.item_profile_id = ? "
        "  AND cd.equipped_slot IN (1130, 1132, 1133, 1136, 1139, 1142, 1143)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Log("armor", "FetchEquippedArmor: prepare failed: %s\n", sqlite3_errmsg(db));
        return out;
    }
    sqlite3_bind_int64(stmt, 1, character_id);
    sqlite3_bind_int  (stmt, 2, item_profile_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EquippedPiece p;
        p.inventoryId   = sqlite3_column_int(stmt, 0);
        p.equippedSlot  = sqlite3_column_int(stmt, 1);
        const char* csv = (const char*)sqlite3_column_text(stmt, 2);
        if (csv) p.modEgidsCsv = csv;
        out.push_back(std::move(p));
    }
    sqlite3_finalize(stmt);
    return out;
}

}  // namespace

void Armor::ClearRecords(ATgPawn* Pawn) {
    if (!Pawn) return;
    Records().erase(Pawn->r_nPawnId);
}

void Armor::RevertDefaultArmor(ATgPawn* Pawn) {
    if (!Pawn) return;
    if (!IsHumanPlayer(Pawn)) return;  // silent for bots — Apply logs the skip

    auto it = Records().find(Pawn->r_nPawnId);
    if (it == Records().end()) {
        // First-ever pass on this pawn: nothing to reverse. Apply will
        // populate the record map; next RCST cycle's Revert will run.
        return;
    }
    auto& records = it->second;

    // Mirror the apply order LIFO with the same two-class dispatch.
    //   class 157 → ApplyBuff(bRemove=1) — exact-match additive subtraction
    //     (header tuple + same calc/amount → subtract); entries that hit
    //     zero get removed from m_EffectBuffInfo. Equal-and-opposite to
    //     apply, so the registry returns to its pre-apply state regardless
    //     of any other layers added in between as long as those layers
    //     don't share our exact (propId, devInst) key.
    //   else → direct m_fRaw subtract of the same delta Apply added.
    int reverted = 0;
    for (auto rit = records.rbegin(); rit != records.rend(); ++rit) {
        const AppliedEntry& e = *rit;
        if (e.classResId == 157) {
            FBuffHeader h{};
            h.nPropId          = e.propId;
            h.nReqCategoryCode = 0;
            h.nReqSkillId      = 0;
            h.nReqDeviceInstId = e.deviceInstId;
            TgPawn__ApplyBuff::Call(Pawn, /*edx=*/nullptr, h, e.calcMethod, e.amount,
                                    /*bRemove=*/1, /*BUFF_SOURCE_TYPE_ITEM=*/1);
        } else {
            UTgProperty* prop = FindProperty(Pawn, e.propId);
            if (!prop) continue;
            const float delta = ComputeRawDelta(e.calcMethod, e.amount, prop->m_fBase);
            if (delta == 0.0f) continue;
            prop->m_fRaw -= delta;
        }
        ++reverted;
    }
    Logger::Log("armor",
        "[Armor/revert] pawn=%p reverted=%d buff entries\n",
        (void*)Pawn, reverted);
    records.clear();
}

void Armor::ApplyDefaultArmor(ATgPawn* Pawn) {
    if (!Pawn) return;
    if (!IsHumanPlayer(Pawn)) {
        Logger::Log("armor",
            "[Armor] skipped pawn=%p (not a human player: PRI=%p r_bIsBot=%d)\n",
            (void*)Pawn,
            (void*)Pawn->PlayerReplicationInfo,
            (int)Pawn->r_bIsBot);
        return;
    }

    // s_nCharacterId is set in SpawnPlayerCharacter:206 BEFORE the pawn's
    // ReapplyCharacterSkillTree call at line 577, so it's reliably populated
    // here. Cast to ATgPawn_Character* because s_nCharacterId lives on the
    // derived class, not the ATgPawn base — IsHumanPlayer above guarantees
    // a player pawn (the only subclass with this field populated).
    // Falling back to 0 means no equipped armor → no buffs applied → empty
    // record map → harmless no-op on revert next cycle.
    const int64_t character_id =
        (int64_t)((ATgPawn_Character*)Pawn)->s_nCharacterId;
    if (character_id == 0) {
        Logger::Log("armor",
            "[Armor] pawn=%p has s_nCharacterId=0 — skipping (no equipped armor)\n",
            (void*)Pawn);
        return;
    }

    // Active loadout slot. SpawnPlayerCharacter + ServerLoadItemProfile both
    // set r_nItemProfileId BEFORE calling RCST (which calls this), so the
    // pawn field is the source of truth at apply time.
    int item_profile_id = ((ATgPawn_Character*)Pawn)->r_nItemProfileId;
    if (item_profile_id < 1 || item_profile_id > 5) item_profile_id = 1;

    const std::vector<EquippedPiece> equipped =
        FetchEquippedArmor(character_id, item_profile_id);
    if (equipped.empty()) {
        Logger::Log("armor",
            "[Armor] pawn=%p char=%lld itemProf=%d has no equipped armor rows — bare stats\n",
            (void*)Pawn, (long long)character_id, item_profile_id);
        return;
    }

    // Collect every egid across every piece in one batch so the effects
    // lookup is a single DB roundtrip rather than 7.
    std::vector<int> allEgids;
    std::vector<std::vector<int>> egidsPerPiece(equipped.size());
    for (size_t pi = 0; pi < equipped.size(); ++pi) {
        egidsPerPiece[pi] = ParseEgidCsv(equipped[pi].modEgidsCsv.c_str());
        allEgids.insert(allEgids.end(), egidsPerPiece[pi].begin(), egidsPerPiece[pi].end());
    }

    const auto effectsByEgid = LookupEffectRows(allEgids);

    auto& records = Records()[Pawn->r_nPawnId];
    records.clear();
    records.reserve(equipped.size() * 7);  // ~1 base + ~6 mods per piece

    int modApplied = 0;
    for (size_t pi = 0; pi < equipped.size(); ++pi) {
        const EquippedPiece& piece = equipped[pi];

        // Per-egid effect fanout. Duplicates in egidsPerPiece produce
        // duplicate ApplyBuff calls — exactly what we want for stacking
        // (a `[rrrrrr]` piece = 6× ApplyBuff(218, …, 0.5)).
        //
        // The CSV starts with the slot's baseline egid (the +10% Health
        // Mod prop-390 effect from the blueprint's Common-tier mod slot,
        // prepended by SeedArmor) followed by the 6 rolled-mod egids
        // from the chosen variant. No hardcoded ApplyBuff(412, 10%) any
        // more — the +10% baseline is now data-driven through this
        // same loop, sourced from `asm_data_set_blueprint_mod_effect_groups`
        // → `asm_data_set_effects` like any other rolled mod.
        //
        // devInst = 0 (wildcard) — armor's stat effects (MaxHP, protection
        // 217/218/219) are GLOBAL player stats with no device context.
        // GetBuffedProperty queries them with devInst = 0; the registry
        // match rule (FUN_109cd890) ignores wildcards-vs-scoped mismatches
        // by only matching `stored devInst > 0` against the query devInst.
        // Scoping armor buffs by inventory_id (as rolled WEAPON mods do)
        // would hide them from the global MaxHP / protection recompute,
        // and the +10% per piece would silently never land. Verified via
        // 1300→1300 (base, should be 2210) regression on 2026-05-30 with
        // per-piece scoping. Skills register with devInst = 0 for the
        // same reason — see Inventory::ApplyRolledModEffects comments.
        for (int egid : egidsPerPiece[pi]) {
            auto fit = effectsByEgid.find(egid);
            if (fit == effectsByEgid.end()) continue;
            for (const EffectRow& r : fit->second) {
                ApplyAndRecord(Pawn, records,
                               r.propId, r.calcMethod, r.baseValue,
                               /*devInst=*/0, r.classResId);
                ++modApplied;
            }
        }

        Logger::Log("armor",
            "[Armor/apply] pawn=%p slot=%d invId=%d mods=%zu (CSV=\"%s\")\n",
            (void*)Pawn, piece.equippedSlot, piece.inventoryId,
            egidsPerPiece[pi].size(),
            piece.modEgidsCsv.c_str());
    }

    Logger::Log("armor",
        "[Armor/apply] pawn=%p char=%lld pieces=%zu mod+%d total=%zu records\n",
        (void*)Pawn, (long long)character_id, equipped.size(),
        modApplied, records.size());

    // Fanout (SetProperty → ApplyProperty → r_nHealthMaximum etc.) is
    // handled by RCST's combined SetProperty pass at the end of
    // ReapplyCharacterSkillTree. RCST already inserts HEALTH_MAX (304)
    // into its touchedPropIds set unconditionally after this call, so
    // the replicated cap (r_fMaxHealth + cached fields) gets refreshed
    // regardless of which props we touched. Protection props (217/218/
    // 219) stay raw-only — CalcProtection (TgEffectGroup.uc:745) reads
    // prop.m_fRaw directly, so the direct-write class-80 branch above
    // is sufficient with no engine fan-out needed.
}
