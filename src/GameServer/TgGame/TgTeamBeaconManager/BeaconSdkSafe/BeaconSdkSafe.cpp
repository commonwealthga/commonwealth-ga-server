#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <vector>

namespace BeaconSdk {

namespace {

UFunction* FindFn(const char* fullName) {
	UFunction* fn = (UFunction*)ObjectCache::Find(fullName);
	if (!fn) {
		Logger::Log("beacon", "BeaconSdk: UFunction not found: %s\n", fullName);
	}
	return fn;
}

// Force FUNC_Native (0x400) ON for the duration of the ProcessEvent so
// dispatch lands at the C++ native instead of the script function body.
// Save/restore the original FunctionFlags around the call — the previous
// pattern `|= ~0x400` cleared nothing and silently set FunctionFlags to
// 0xFFFFFFFF on every invocation, permanently corrupting the UFunction.
inline void CallNative(UObject* obj, UFunction* fn, void* parms) {
	const uint32_t orig = fn->FunctionFlags;
	fn->FunctionFlags = orig | 0x400;
	obj->ProcessEvent(fn, parms, nullptr);
	fn->FunctionFlags = orig;
}

}  // namespace

bool RegisterBeacon(ATgTeamBeaconManager* mgr, ATgDeploy_Beacon* pBeacon, bool bDeployed) {
	if (!mgr) return false;
	static UFunction* fn = FindFn("Function TgGame.TgTeamBeaconManager.RegisterBeacon");
	if (!fn) return false;

	struct Parms {
		ATgDeploy_Beacon* pBeacon;
		uint32_t          bDeployed;
		uint32_t          ReturnValue;
	} parms = { pBeacon, bDeployed ? 1u : 0u, 0u };

	CallNative(mgr, fn, &parms);
	return (parms.ReturnValue & 1u) != 0;
}

bool CheckBeacon(ATgTeamBeaconManager* mgr, bool bAttemptRespawn) {
	if (!mgr) return false;
	static UFunction* fn = FindFn("Function TgGame.TgTeamBeaconManager.CheckBeacon");
	if (!fn) return false;

	struct Parms {
		uint32_t bAttemptRespawn;
		uint32_t ReturnValue;
	} parms = { bAttemptRespawn ? 1u : 0u, 0u };

	CallNative(mgr, fn, &parms);
	return (parms.ReturnValue & 1u) != 0;
}

bool TeamHasBeaconCarrier(ATgTeamBeaconManager* mgr) {
	if (!mgr || !mgr->r_TaskForce) return false;

	// Mirrors the carrier scan inside the binary's CheckBeacon (0x109f0340)
	// EXACTLY — same list, same hops, same predicate — so our spawn gate can
	// never disagree with the state machine that owns the decision:
	//
	//   for i in 0..r_TaskForce->GetPlayerCount():
	//       pri  = r_TaskForce->GetPlayer(i)        // m_TeamPlayers[i].pPrep
	//       ctl  = Cast_AController(pri->Owner)     // AActor::Owner @ +0x98
	//       pawn = Cast_ATgPawn(ctl->Pawn)          // AController::Pawn @ +0x1CC
	//       if (pawn->IsCarryingBeacon()) -> carrier
	//
	// m_TeamBots is deliberately NOT scanned — CheckBeacon doesn't either.
	const int count = mgr->r_TaskForce->m_TeamPlayers.Count;
	for (int i = 0; i < count; ++i) {
		ATgRepInfo_Player* pri = mgr->r_TaskForce->m_TeamPlayers.Data[i].pPrep;
		if (!pri || !pri->Owner) continue;
		if (!ObjectClassCache::ClassNameContains(pri->Owner, "Controller")) continue;
		AController* ctl = (AController*)pri->Owner;
		if (!ctl->Pawn) continue;
		if (!ObjectClassCache::ClassNameContains(ctl->Pawn, "TgPawn")) continue;
		if (PawnHoldsBeaconDevice((ATgPawn*)ctl->Pawn)) return true;
	}
	return false;
}

bool ShouldSpawnBeacon(ATgTeamBeaconManager* mgr) {
	if (!mgr) return false;

	// Deliberately NOT a call to the binary's ShouldSpawnBeacon native
	// (0x109ee6c0). That native is `CheckBeacon(false) == false`, and our only
	// callers run from INSIDE CheckBeacon's own frame (it dispatches
	// SpawnNewBeaconForTeam via vtable[0x374]) — so using it would re-enter the
	// state machine mid-update through ProcessEvent. It also carries a
	// `s_bUsingBeaconInventory && !s_HexItem -> false` clause for the
	// hex/Territory beacon mechanic we don't implement.
	//
	// We evaluate the same predicate side-effect-free instead: no live beacon
	// AND nobody on the team holding the slot-11 pickup device. Identical to
	// what CheckBeacon's respawn branch tests, with no re-entrancy and no
	// dependency on a UFunction lookup.
	if (mgr->r_Beacon != nullptr) return false;
	return !TeamHasBeaconCarrier(mgr);
}

bool PawnHoldsBeaconDevice(ATgPawn* Pawn) {
	if (!Pawn) return false;
	ATgDevice* dev = Pawn->m_EquippedDevices[11];
	if (!dev) return false;
	// m_bIsBeaconPlacing = bit 0x10000 of the dword at TgDevice+0x22C
	// (IsABeaconPlacingDevice @ 0x10a19a40 tests *(byte*)(dev+0x22E) & 1).
	return (*(uint32_t*)((char*)dev + 0x22C) & 0x10000u) != 0;
}

int ReapOrphanBeacons(ATgTeamBeaconManager* mgr) {
	if (!mgr || !mgr->r_TaskForce) return 0;

	// eventDestroyIt re-enters UC (Destroyed -> UnRegisterBeacon ->
	// CheckBeacon). Never let that path re-enter the reaper.
	static bool s_reaping = false;
	if (s_reaping) return 0;

	ATgGame* game = (ATgGame*)Globals::Get().GGameInfo;
	ATgRepInfo_Game* gri = game ? (ATgRepInfo_Game*)game->GameReplicationInfo : nullptr;
	if (!gri) return 0;

	// Snapshot first: eventDestroyIt mutates gri->m_Deployables (the
	// RegisterDeployableInGRI compaction pass drops dead entries), so we must
	// not be iterating it while destroying.
	std::vector<ATgDeploy_Beacon*> orphans;
	for (int i = 0; i < gri->m_Deployables.Count; ++i) {
		ATgDeployable* dep = gri->m_Deployables.Data[i];
		if (!dep) continue;
		if (dep->r_nDeployableId != 36) continue;          // exit beacon only (entrance = 48)
		if (dep == (ATgDeployable*)mgr->r_Beacon) continue; // the keeper
		if (dep->m_bInDestroyedState) continue;
		// Team match via the DRI — both spawn paths wire r_TaskforceInfo.
		if (!dep->r_DRI || dep->r_DRI->r_TaskforceInfo != mgr->r_TaskForce) continue;
		orphans.push_back((ATgDeploy_Beacon*)dep);
	}
	if (orphans.empty()) return 0;

	s_reaping = true;
	for (ATgDeploy_Beacon* orphan : orphans) {
		Logger::Log("beacon",
			"ReapOrphanBeacons: destroying untracked beacon 0x%p (tf=%d) — mgr=0x%p keeps 0x%p\n",
			orphan, (int)mgr->r_TaskForce->r_nTaskForce, mgr, mgr->r_Beacon);
		// s_bWasPickedUp=0 so this reads as a destruction, not a pickup.
		orphan->s_bWasPickedUp = 0;
		orphan->eventDestroyIt(0);
	}
	s_reaping = false;
	return (int)orphans.size();
}

void PopulateBeaconFactoryList(ATgTeamBeaconManager* mgr) {
	if (!mgr) return;
	static UFunction* fn = FindFn("Function TgGame.TgTeamBeaconManager.PopulateBeaconFactoryList");
	if (!fn) return;
	// No-param native, no struct fields to fight over.
	CallNative(mgr, fn, nullptr);
}

ATgTeamBeaconManager* FindBeaconManagerByTaskForce(int taskforceNum) {
	if (UObject::GObjObjects() == nullptr) return nullptr;
	UClass* mgrCls = ClassPreloader::GetClass("Class TgGame.TgTeamBeaconManager");
	if (!mgrCls) return nullptr;
	for (int i = 0; i < UObject::GObjObjects()->Count; ++i) {
		UObject* obj = UObject::GObjObjects()->Data[i];
		if (!obj || obj->Class != mgrCls) continue;
		ATgTeamBeaconManager* mgr = (ATgTeamBeaconManager*)obj;
		if (mgr->r_TaskForce && mgr->r_TaskForce->r_nTaskForce == taskforceNum) {
			return mgr;
		}
	}
	return nullptr;
}

void SetCollision(AActor* actor, bool bColActors, bool bBlockActors, bool bIgnoreEncroachers) {
	if (!actor) return;
	static UFunction* fn = FindFn("Function Engine.Actor.SetCollision");
	if (!fn) return;

	struct Parms {
		uint32_t bColActors;
		uint32_t bBlockActors;
		uint32_t bIgnoreEncroachers;
	} parms = {
		bColActors        ? 1u : 0u,
		bBlockActors      ? 1u : 0u,
		bIgnoreEncroachers? 1u : 0u
	};

	CallNative(actor, fn, &parms);
}

void SetCollisionType(AActor* actor, unsigned char newCollisionType) {
	if (!actor) return;
	static UFunction* fn = FindFn("Function Engine.Actor.SetCollisionType");
	if (!fn) return;
	// Single byte param; a 4-byte cell covers the frame safely.
	uint32_t parm = newCollisionType;
	CallNative(actor, fn, &parm);
}

void SetLocation(AActor* actor, const FVector& newLocation) {
	if (!actor) return;
	static UFunction* fn = FindFn("Function Engine.Actor.SetLocation");
	if (!fn) return;
	struct Parms {
		FVector  NewLocation;
		uint32_t ReturnValue;
	} parms = { newLocation, 0 };
	CallNative(actor, fn, &parms);
}

void DropCarriedBeacon(ATgPawn* Pawn) {
	if (!Pawn || !Pawn->PlayerReplicationInfo) return;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	// Gate on the ACTUAL carry state, not on mgr->r_BeaconHolder. See the
	// header comment: r_BeaconHolder means "carrier" only in CheckBeacon's
	// PICKED_UP branch; for any live beacon it is overwritten with the
	// DEPLOYER's PRI, so the old `r_BeaconHolder == pri` gate silently stopped
	// firing as soon as anyone else deployed or picked up — leaving slot 11
	// populated for the rest of the match.
	if (!PawnHoldsBeaconDevice(Pawn)) return;

	Logger::Log("beacon",
		"DropCarriedBeacon: pawn=0x%p pri=0x%p holds slot-11 beacon device — clearing + CheckBeacon\n",
		Pawn, pri);

	// Inventory device removal first — UC TgDevice.uc:677 invokes
	// CheckBeacon on inventory change, but we follow up with an explicit
	// call to guarantee the respawn fires even if the inventory hook
	// path doesn't trigger during the Dying / Destroyed teardown.
	ATgInventoryManager* invMgr = (ATgInventoryManager*)Pawn->InvManager;
	if (invMgr) {
		TgInventoryManager__NonPersistRemoveDevice::Call(invMgr, nullptr, 11);
	}

	// Re-evaluate every manager this pawn could have been the carrier for:
	// its own team's, plus any that still names this PRI as holder (stale
	// after a team change). bAttemptRespawn=true so the beacon comes back at
	// the original-priority factory now that nobody is carrying it.
	//
	// Deliberately NOT a blanket CheckBeacon on both managers — that could
	// change respawn timing for the opposing team as a side effect of this
	// pawn dying.
	ATgTeamBeaconManager* own = (pri->r_TaskForce ? pri->r_TaskForce->r_BeaconManager : nullptr);
	if (own) CheckBeacon(own, true);

	ATgTeamBeaconManager* managers[2] = {
		(GTeamsData.Attackers ? GTeamsData.Attackers->r_BeaconManager : nullptr),
		(GTeamsData.Defenders ? GTeamsData.Defenders->r_BeaconManager : nullptr),
	};
	for (ATgTeamBeaconManager* mgr : managers) {
		if (!mgr || mgr == own) continue;
		if (mgr->r_BeaconHolder != pri) continue;
		CheckBeacon(mgr, true);
	}
}

void ReleaseBeaconForTeamChange(ATgPawn* Pawn) {
	if (!Pawn || !Pawn->PlayerReplicationInfo) return;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	// Case A: carrying — drop + respawn for the (still-current) old team.
	DropCarriedBeacon(Pawn);

	// Case B: this pawn deployed the team's world beacon. Sever its personal
	// ownership (Instigator + DRI r_InstigatorInfo) but keep the beacon with
	// the taskforce (r_bOwnedByTaskforce / r_TaskforceInfo untouched) so the
	// old team keeps its forward spawn after the carrier leaves.
	ATgTeamBeaconManager* managers[2] = {
		(GTeamsData.Attackers ? GTeamsData.Attackers->r_BeaconManager : nullptr),
		(GTeamsData.Defenders ? GTeamsData.Defenders->r_BeaconManager : nullptr),
	};
	for (ATgTeamBeaconManager* mgr : managers) {
		if (!mgr) continue;
		ATgDeploy_Beacon* beacon = mgr->r_Beacon;
		if (!beacon) continue;

		const bool ownedByPawn =
			(beacon->Instigator == (APawn*)Pawn) ||
			(beacon->r_DRI && beacon->r_DRI->r_InstigatorInfo == pri);
		if (!ownedByPawn) continue;

		Logger::Log("beacon",
			"ReleaseBeaconForTeamChange: pawn=0x%p deployed beacon 0x%p on mgr=0x%p — clearing personal ownership\n",
			Pawn, beacon, mgr);

		beacon->Instigator = nullptr;
		if (beacon->r_DRI) {
			beacon->r_DRI->r_InstigatorInfo = nullptr;
			beacon->r_DRI->bNetDirty       = 1;
			beacon->r_DRI->bForceNetUpdate = 1;
		}
	}
}

ATgRepInfo_TaskForce* GetTaskForce(ATgRepInfo_Game* GRI, int nTaskForceNum, bool bCreate) {
	if (!GRI) return nullptr;
	static UFunction* fn = FindFn("Function TgGame.TgRepInfo_Game.GetTaskForce");
	if (!fn) return nullptr;

	struct Parms {
		int                   nTaskForceNum;
		uint32_t              bCreate;
		ATgRepInfo_TaskForce* ReturnValue;
	} parms = { nTaskForceNum, bCreate ? 1u : 0u, nullptr };

	CallNative(GRI, fn, &parms);
	return parms.ReturnValue;
}

}  // namespace BeaconSdk
