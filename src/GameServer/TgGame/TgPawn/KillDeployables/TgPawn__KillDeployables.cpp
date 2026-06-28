#include "src/GameServer/TgGame/TgPawn/KillDeployables/TgPawn__KillDeployables.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Database/Database.hpp"
#include <sqlite3.h>
#include <unordered_set>

// Native UTgPawn::execKillDeployables @ 0x109c0870 is a void stub. UC declares
//     native function KillDeployables(bool bAll);
// and the exec stub makes UC calls to it no-ops. Callers:
//     TgPawn.uc:3089  KillDeployables(true)   ← round-end / destroy-all path
//     TgPawn.uc:4378  KillDeployables(false)  ← regular death: only mark-on-death
//
// This replaces the no-op with the logic UC would have had: iterate
// s_SelfDeployableList, DestroyIt each entry that either (a) the caller asked
// for blanket destruction via bAll, or (b) has m_bDestroyOnOwnerDeathFlag
// (from asm_data_set_deployables.destroy_on_owner_death_flag via
// InitializeFromDeployableDat's cfg+0x58 bit0).
//
// Intentionally does NOT call the original (it would be a no-op anyway).
void __fastcall TgPawn__KillDeployables::Call(ATgPawn* Pawn, void* edx, unsigned long bAll) {
	if (!Pawn) return;

	const int count = Pawn->s_SelfDeployableList.Count;
	Logger::Log("debug",
		"[KillDeployables] pawn=0x%p pawnId=%d bAll=%d listCount=%d\n",
		Pawn, (int)Pawn->r_nPawnId, (int)bAll, count);

	// Walk in reverse in case the destroy path unlinks entries from the list
	// (it shouldn't on the server path — DestroyIt sets flags and bumps
	// r_nReplicateDestroyIt without mutating s_SelfDeployableList — but the
	// reverse walk is robust either way and matches UE3's foreach semantics
	// against a destroyed-during-iteration actor).
	for (int i = count - 1; i >= 0; --i) {
		ATgDeployable* dep = Pawn->s_SelfDeployableList.Data[i];
		if (!dep) continue;

		// Beacons (deployable_id 36) are TEAM resources owned by
		// TgTeamBeaconManager, not personal deployables. Their lifecycle is
		// driven by CheckBeacon / RegisterBeacon / UnRegisterBeacon and
		// must NOT be tied to the deployer's pawn lifetime — a player who
		// throws a beacon and then dies (or is fully destroyed via the
		// KillAllOwnedPets event that fires bAll=1) should leave the beacon
		// standing for their team.
		if (dep->r_nDeployableId == 36) {
			Logger::Log("debug",
				"  [KillDeployables] skip beacon [%d] 0x%p (team resource — managed by BeaconManager)\n",
				i, dep);
			continue;
		}

		const bool shouldDestroy = bAll || dep->m_bDestroyOnOwnerDeathFlag;
		if (!shouldDestroy) {
			Logger::Log("debug",
				"  [KillDeployables] keep [%d] 0x%p deployableId=%d (destroy_on_owner_death=0)\n",
				i, dep, dep->r_nDeployableId);
			continue;
		}
		if (dep->m_bInDestroyedState) {
			Logger::Log("debug",
				"  [KillDeployables] skip [%d] 0x%p deployableId=%d (already destroyed)\n",
				i, dep, dep->r_nDeployableId);
			continue;
		}

		Logger::Log("debug",
			"  [KillDeployables] DestroyIt [%d] 0x%p deployableId=%d\n",
			i, dep, dep->r_nDeployableId);

		// UC `Dep.eventDestroyIt(false)` — runs the full UC DestroyIt event
		// which: stops fire, sets LifeSpan, hides mesh, swaps to destroyed
		// mesh, flips m_bInDestroyedState, bumps r_nReplicateDestroyIt to
		// trigger the client-side repnotify.
		dep->eventDestroyIt(0 /* bSkipFx */);
	}

	// UC does `s_SelfDeployableList.Length = 0;` — reset the Count. Data
	// buffer stays allocated for reuse by future spawns on this pawn. The
	// destroyed actors themselves will be GC'd after LifeSpan expires.
	Pawn->s_SelfDeployableList.Count = 0;
}

// Deployable IDs with pickup_device_id > 0 in asm_data_set_deployables are
// B-keybind-pickupable team assets (hubs, beacons, platforms, etc.) that
// must survive profile-switch and team-change. Loaded once from the DB.
static const std::unordered_set<int>& GetPickupableIds() {
	static std::unordered_set<int> ids;
	static bool loaded = false;
	if (loaded) return ids;
	loaded = true;
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
		"SELECT deployable_id FROM asm_data_set_deployables WHERE pickup_device_id > 0",
		-1, &stmt, nullptr) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW)
			ids.insert(sqlite3_column_int(stmt, 0));
		sqlite3_finalize(stmt);
	}
	return ids;
}

// Walk gri->m_Deployables (global, survives pawn respawn) and PawnList (for
// deployed bot pawns like the Grizzly drone). Called on team-change and
// profile-switch so deployables placed before a prior death are also caught.
void TgPawn__KillDeployables::KillAllOwned(ATgPawn* Pawn) {
	if (!Pawn) return;

	AWorldInfo* wi = Pawn->WorldInfo;
	if (!wi || !wi->GRI) return;
	ATgRepInfo_Game* gri = (ATgRepInfo_Game*)wi->GRI;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	// Destroy ATgDeployables in the global list that belong to this player.
	const auto& pickupable = GetPickupableIds();
	Logger::Log("deployables",
		"[KillAllOwned] pawn=0x%p pri=0x%p gri->m_Deployables.Count=%d\n",
		Pawn, pri, gri->m_Deployables.Count);
	for (int i = gri->m_Deployables.Count - 1; i >= 0; --i) {
		ATgDeployable* dep = gri->m_Deployables.Data[i];
		if (!dep) continue;
		Logger::Log("deployables",
			"[KillAllOwned]   [%d] dep=0x%p id=%d destroyed=%d DRI=0x%p instigator=0x%p\n",
			i, dep, dep->r_nDeployableId, (int)dep->m_bInDestroyedState,
			dep->r_DRI, dep->r_DRI ? dep->r_DRI->r_InstigatorInfo : nullptr);
		if (dep->m_bInDestroyedState) continue;
		if (pickupable.count(dep->r_nDeployableId)) continue; // B-keybind team asset
		if (!dep->r_DRI || dep->r_DRI->r_InstigatorInfo != pri) continue;
		Logger::Log("deployables",
			"[KillAllOwned] DestroyIt deployable=0x%p id=%d\n",
			dep, dep->r_nDeployableId);
		dep->eventDestroyIt(0);
	}

	// Kill deployed bot pawns (e.g. Grizzly drone).
	// Log all non-player pawns so we can identify the correct ownership field.
	for (APawn* p = wi->PawnList; p != nullptr; p = p->NextPawn) {
		if (p == (APawn*)Pawn || p->Health <= 0 || p->bDeleteMe) continue;
		if (ObjectClassCache::ClassNameContains((UObject*)p->Controller, "PlayerController")) continue;
		Logger::Log("deployables",
			"[KillAllOwned] bot pawn=0x%p Instigator=0x%p PRI=0x%p\n",
			p, p->Instigator, p->PlayerReplicationInfo);
		if (p->Instigator != (APawn*)Pawn) continue;
		Logger::Log("deployables",
			"[KillAllOwned] KilledBy bot pawn=0x%p\n", p);
		p->eventKilledBy((APawn*)Pawn);
	}

	Pawn->s_SelfDeployableList.Count = 0;
}
