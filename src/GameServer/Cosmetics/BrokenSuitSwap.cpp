#include "src/pch.hpp"
#include "src/GameServer/Cosmetics/BrokenSuitSwap.hpp"

#include "src/Database/Database.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Cosmetics/CosmeticEquip.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/UserPreferences/UserPreferences.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <unordered_map>

namespace BrokenSuitSwap {

namespace {

// Hand-curated replacement pairs, cosmetic item_id → cosmetic item_id
// (asm_data_set_items.item_id, subtypes 1006/1007/1008 — see
// cosmetic-items.md for the full catalog). Applies to both suit and helmet
// slots; the item id decides which fields get rewritten.
// `to = 0` = show as wearing NOTHING (suit → class-default body, helmet →
// bare head).
constexpr struct { int from; int to; } kSwapPairs[] = {
	// assault
	{7015, 6780},
	{7013, 6780},
	{7012, 6780},
	{7432, 4736},
	{6314, 4736},
	{7149, 4736},

	// medic
	{7449, 3253},
	{6316, 3253},
	{3709, 3253},

	// recon
	{7450, 3193},
	{6315, 3193},

	// robotics
	{7451, 5354},
	{6312, 5354},

	// all
	{7549, 0},
	{7551, 0},
	{7569, 0},
};

struct Replacement {
	int itemId;
	int meshAsmMale;    // asm_data_set_item_mesh_asm_groups asm_id, gender 853
	int meshAsmFemale;  // gender 852; falls back to the male row when absent
};

// from item_id → resolved replacement. Built once, then read-only.
std::unordered_map<int, Replacement> g_swaps;
bool g_resolved = false;

// One row per gender for a cosmetic item. Gender ids in the data: 852 Female,
// 853 Male (near-unused), 854 Generic (what male pawns actually wear) — so
// "male" here means any non-female row; each side falls back to the other
// when its rows are absent (same spirit as LookupMeshAsmIdForCosmetic).
void LookupMeshPair(sqlite3* db, int itemId, int& outMale, int& outFemale) {
	outMale = 0;
	outFemale = 0;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT gender_type_value_id, asm_id "
	        "FROM asm_data_set_item_mesh_asm_groups WHERE item_id = ?",
	        -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, itemId);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			const int gender = sqlite3_column_int(stmt, 0);
			const int asmId  = sqlite3_column_int(stmt, 1);
			if (gender == GA_G::GENDER_TYPE_ID_FEMALE) {
				if (outFemale == 0) outFemale = asmId;
			} else {
				if (outMale == 0) outMale = asmId;
			}
		}
		sqlite3_finalize(stmt);
	}
	if (outFemale == 0) outFemale = outMale;
	if (outMale == 0) outMale = outFemale;
}

// Resolve kSwapPairs' replacement mesh asm ids. One-time; a handful of
// indexed local sqlite reads. After this every lookup is memory-only.
void EnsureResolved() {
	if (g_resolved) return;
	sqlite3* db = Database::GetConnection();
	if (!db) return;  // retry on next call
	g_resolved = true;
	for (const auto& pair : kSwapPairs) {
		Replacement rep{};
		rep.itemId = pair.to;
		if (pair.to == 0) {
			// "Show as wearing nothing" sentinel — mesh resolved per-wearer
			// (class-default body) in ApplyReplacements, nothing to look up.
			g_swaps[pair.from] = rep;
			Logger::Log("brokensuits", "[BrokenSuitSwap] map %d -> hide\n", pair.from);
			continue;
		}
		LookupMeshPair(db, pair.to, rep.meshAsmMale, rep.meshAsmFemale);
		if (rep.meshAsmMale == 0) {
			Logger::Log("brokensuits",
				"[BrokenSuitSwap] item %d -> %d: no mesh asm rows for replacement, "
				"flair-only swap\n", pair.from, pair.to);
		}
		g_swaps[pair.from] = rep;
		Logger::Log("brokensuits",
			"[BrokenSuitSwap] map %d -> %d (mesh M=%d F=%d)\n",
			pair.from, pair.to, rep.meshAsmMale, rep.meshAsmFemale);
	}
}

const Replacement* Find(int itemId) {
	if (itemId <= 0) return nullptr;
	auto it = g_swaps.find(itemId);
	return (it != g_swaps.end()) ? &it->second : nullptr;
}

} // namespace

bool HasMappedCosmetic(const FCustomCharacterAssembly& a) {
	EnsureResolved();
	if (g_swaps.empty()) return false;
	return Find(a.SuitFlairId) != nullptr || Find(a.HeadFlairId) != nullptr;
}

bool ViewerHidesBrokenSuits(void* Connection) {
	if (!Connection) return false;
	auto it = GClientConnectionsData.find((int32_t)Connection);
	if (it == GClientConnectionsData.end()) return false;
	const int64_t userId = it->second.PlayerInfo.user_id;
	if (userId <= 0) return false;
	return !UserPreferences::GetBool(userId, kPrefKey, kPrefDefault);
}

bool ApplyReplacements(FCustomCharacterAssembly& a, int profileId, SavedFields& saved) {
	saved.SuitFlairId  = a.SuitFlairId;
	saved.SuitMeshId   = a.SuitMeshId;
	saved.HeadFlairId  = a.HeadFlairId;
	saved.HelmetMeshId = a.HelmetMeshId;

	const bool isFemale = a.nGenderTypeId == GA_G::GENDER_TYPE_ID_FEMALE;
	bool changed = false;

	if (const Replacement* rep = Find(a.SuitFlairId)) {
		if (rep->itemId == 0) {
			// "No suit": same state ClearSlot/ApplyClassVisualFallback produce —
			// no flair overlay, class-default body mesh.
			a.SuitFlairId = -1;
			const int mesh = CosmeticEquip::ClassDefaultSuitAsmId((uint32_t)profileId, isFemale);
			if (mesh > 0) a.SuitMeshId = mesh;
		} else {
			a.SuitFlairId = rep->itemId;
			const int mesh = isFemale ? rep->meshAsmFemale : rep->meshAsmMale;
			if (mesh > 0) a.SuitMeshId = mesh;
		}
		changed = true;
	}
	if (const Replacement* rep = Find(a.HeadFlairId)) {
		if (rep->itemId == 0) {
			// "No helmet": bare head (ClearSlot slot-12 state).
			a.HeadFlairId  = -1;
			a.HelmetMeshId = -1;
		} else {
			a.HeadFlairId = rep->itemId;
			const int mesh = isFemale ? rep->meshAsmFemale : rep->meshAsmMale;
			if (mesh > 0) a.HelmetMeshId = mesh;
		}
		changed = true;
	}
	return changed;
}

void RestoreAssembly(FCustomCharacterAssembly& a, const SavedFields& saved) {
	a.SuitFlairId  = saved.SuitFlairId;
	a.SuitMeshId   = saved.SuitMeshId;
	a.HeadFlairId  = saved.HeadFlairId;
	a.HelmetMeshId = saved.HelmetMeshId;
}

} // namespace BrokenSuitSwap
