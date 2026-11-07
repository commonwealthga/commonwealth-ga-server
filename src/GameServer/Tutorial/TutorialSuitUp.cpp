#include "src/GameServer/Tutorial/TutorialSuitUp.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/TgGame/TgPawn/ServerOnEquipCharDevices/TgPawn__ServerOnEquipCharDevices.hpp"
#include "src/GameServer/Cosmetics/SuitRebuildKick.hpp"
#include "src/Database/Database.hpp"
#include "src/Shared/SlotUnlock.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

struct OwnLook {
	FCustomCharacterAssembly assembly;
	int itemProfileId = 1;
};

std::map<int64_t, OwnLook> s_own;
std::set<int64_t> s_suited;

// Skylark_P runs lv3 -> lv4 (its Kismet SetPlayerLevel 3/4; the control server raises to 3 at chapter start); the slot unlock list opens
// Offhand 1 at 3, Specialty + Offhand 2 at 4.
constexpr int kSkylarkLevel = 3;  // Skylark is played at level 3 (Offhand 1); Specialty + Offhand 2 at Agency Zero lv4

// Tutorial chapter levels. Each chapter's Kismet
// TgSeqAct_SetPlayerLevel starts it at 2/3/4 (and ends it one higher); the
// control server raises the DB xp at the chapter hand-off, this floor makes
// the chapter's slots open even when the DB lags (old CS, node seed).
int TutorialChapterLevelFloor() {
	const std::string m = Config::GetMapNameChar();
	return (m == "Adrenaline_P") ? 2 : (m == "Skylark_P") ? 3 : (m == "AgencyZero_P") ? 4 : 0;
}
const bool kLevelFloorRegistered = (SlotUnlock::g_LevelFloor = &TutorialChapterLevelFloor, true);

bool IsLabSuit(int meshAsmId) { return meshAsmId == 1752 || meshAsmId == 1779; }

std::vector<int> ParseMods(const unsigned char* csv) {
	std::vector<int> mods;
	const char* p = reinterpret_cast<const char*>(csv);
	while (p && *p) {
		while (*p == ',' || *p == ' ') ++p;
		if (!*p) break;
		char* end = nullptr;
		long v = std::strtol(p, &end, 10);
		if (end == p) break;
		mods.push_back((int)v);
		p = end;
	}
	return mods;
}

void ApplyOwnLook(ATgPawn* Pawn, int64_t charId) {
	FCustomCharacterAssembly& a = ((ATgPawn_Character*)Pawn)->r_CustomCharacterAssembly;
	auto it = s_own.find(charId);
	if (it != s_own.end()) {
		a = it->second.assembly;
	} else {
		a.bHideHelmet = 0;  // no snapshot: keep the Kismet's mesh 0 = default suit
	}
	a.bValidCustomAssembly = 1;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (pri) {
		pri->r_CustomCharacterAssembly = a;
		pri->bNetDirty = 1;
		pri->bForceNetUpdate = 1;
	}
	Pawn->bNetDirty = 1;
	Pawn->bForceNetUpdate = 1;
	SuitRebuildKick::Schedule(Pawn);  // late ApplyPawnSetup heals a failed mesh load
}

// The class kit the Lab Suit spawn left out (offhands, specialty, morale,
// consumable), from the agent's own equipped rows, like SpawnPlayerCharacter.
int EquipClassKit(ATgPawn* Pawn, int64_t charId) {
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	auto it = s_own.find(charId);
	const int profile = (it != s_own.end()) ? it->second.itemProfileId : 1;
	const int level = std::max(SlotUnlock::CharacterLevel(db, charId), kSkylarkLevel);
	struct Row { int deviceId, slot, quality, inventoryId; std::vector<int> mods; };
	std::vector<Row> rows;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT i.device_id, d.equipped_slot, i.quality, i.id, i.mod_effect_group_ids "
			"FROM ga_character_devices d "
			"JOIN ga_players_inventory i ON i.id = d.inventory_id "
			"WHERE d.character_id = ? AND d.item_profile_id = ? AND i.device_id > 0 "
			"ORDER BY d.equipped_slot",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		Logger::Log("kismet", "[Tutorial] suit-up kit query failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, charId);
	sqlite3_bind_int  (stmt, 2, profile);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		rows.push_back({ sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1),
			sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3),
			ParseMods(sqlite3_column_text(stmt, 4)) });
	}
	sqlite3_finalize(stmt);

	ATgInventoryManager* im = (ATgInventoryManager*)Pawn->InvManager;
	const int itemCount = im ? im->r_ItemCount : 0;
	int equipped = 0;
	for (const Row& r : rows) {
		if (!TgPawn__ServerOnEquipCharDevices::IsTutorialStripSlot(r.slot)) continue;  // spawned with it
		bool held = false;
		for (const EquippedEntry& e : Inventory::GetEquipped(Pawn)) {
			if (e.slot == r.slot) { held = true; break; }
		}
		if (held) continue;  // melee / ranged / jetpack from the earlier chapters
		if (!SlotUnlock::IsEquipPointUnlocked(db, level, r.slot)
				|| !SlotUnlock::IsItemUsable(db, level, r.deviceId, 0)) {
			Logger::Log("kismet", "[Tutorial] suit-up char=%lld slot=%d device=%d locked at lv%d\n",
				(long long)charId, r.slot, r.deviceId, level);
			continue;
		}
		if (Inventory::Equip(Pawn, r.deviceId, r.slot, r.quality, r.inventoryId, r.mods)) {
			++equipped;
			Logger::Log("kismet", "[Tutorial] suit-up char=%lld slot=%d device=%d inv=%d\n",
				(long long)charId, r.slot, r.deviceId, r.inventoryId);
		}
	}
	if (equipped > 0) {
		Inventory::Finalize(Pawn);
		// Equip ++'s r_ItemCount even for a bag item the client already
		// holds; keep the client's record count (InvManager.IsValid gate).
		if (im) {
			im->r_ItemCount = itemCount;
			im->bNetDirty = 1;
			im->bForceNetUpdate = 1;
		}
	}
	return equipped;
}

}  // namespace

namespace TutorialSuitUp {

void SaveOwnLook(int64_t charId, const FCustomCharacterAssembly& own, int itemProfileId) {
	if (charId <= 0) return;
	if (s_own.count(charId)) return;  // first spawn holds the DB look; respawns keep it
	OwnLook o;
	o.assembly = own;
	o.itemProfileId = itemProfileId;
	s_own[charId] = o;
}

bool IsSuitedUp(int64_t charId) { return s_suited.count(charId) != 0; }

bool InLabSuitPhase(ATgPawn* Pawn) {
	if (!Pawn || Pawn->bDeleteMe) return false;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	ATgRepInfo_Game* gri = Game ? (ATgRepInfo_Game*)Game->GameReplicationInfo : nullptr;
	if (!gri || !gri->r_bIsTutorialMap) return false;
	const std::string m = Config::GetMapNameChar();
	if (m == "AgencyZero_P") return false;
	if (m != "Skylark_P") return true;  // Inception_ALL / Inception_3_TEMP / Adrenaline_P
	return !IsSuitedUp((int64_t)((ATgPawn_Character*)Pawn)->s_nCharacterId);
}

void ApplyLabSuit(ATgPawn* Pawn, const char* why) {
	if (!InLabSuitPhase(Pawn)) return;
	FCustomCharacterAssembly& a = ((ATgPawn_Character*)Pawn)->r_CustomCharacterAssembly;
	const int want = (a.nGenderTypeId == 852) ? 1779 : 1752;
	const bool changed = a.SuitMeshId != want || !a.bHideHelmet || a.SuitFlairId != 0;
	a.SuitMeshId = want;
	a.bHideHelmet = 1;
	a.SuitFlairId = 0;
	a.bValidCustomAssembly = 1;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (pri) {
		pri->r_CustomCharacterAssembly = a;
		pri->bNetDirty = 1;
		pri->bForceNetUpdate = 1;
	}
	Pawn->bNetDirty = 1;
	Pawn->bForceNetUpdate = 1;
	if (changed) SuitRebuildKick::Schedule(Pawn);
	Logger::Log("kismet", "[Tutorial] Lab Suit kept after %s char=%lld suit=%d changed=%d\n", why,
		(long long)((ATgPawn_Character*)Pawn)->s_nCharacterId, want, (int)changed);
}

void OnSetPlayerMesh(ATgPawn* Pawn, int MeshAsmId) {
	if (!Pawn || Pawn->bDeleteMe) return;
	if (std::string(Config::GetMapNameChar()) != "Skylark_P") return;
	const int64_t charId = (int64_t)((ATgPawn_Character*)Pawn)->s_nCharacterId;
	if (charId <= 0) return;
	if (MeshAsmId == 0) {
		const bool first = s_suited.insert(charId).second;
		ApplyOwnLook(Pawn, charId);
		const int kit = first ? EquipClassKit(Pawn, charId) : 0;
		Logger::Log("kismet", "[Tutorial] Skylark suit-up char=%lld suit=%d kit=%d first=%d\n",
			(long long)charId, (int)((ATgPawn_Character*)Pawn)->r_CustomCharacterAssembly.SuitMeshId, kit, (int)first);
	} else if (IsLabSuit(MeshAsmId) && IsSuitedUp(charId)) {
		// spawn-time Lab Suit Kismet re-run after a respawn: stay suited up
		ApplyOwnLook(Pawn, charId);
		Logger::Log("kismet", "[Tutorial] Skylark Lab Suit after suit-up ignored char=%lld\n", (long long)charId);
	}
}

}  // namespace TutorialSuitUp
