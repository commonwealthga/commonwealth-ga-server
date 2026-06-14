#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"

#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

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

void DropCarriedBeacon(ATgPawn* Pawn) {
	if (!Pawn || !Pawn->PlayerReplicationInfo) return;
	ATgRepInfo_Player* pri = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;

	ATgTeamBeaconManager* managers[2] = {
		(GTeamsData.Attackers ? GTeamsData.Attackers->r_BeaconManager : nullptr),
		(GTeamsData.Defenders ? GTeamsData.Defenders->r_BeaconManager : nullptr),
	};
	for (ATgTeamBeaconManager* mgr : managers) {
		if (!mgr || mgr->r_BeaconHolder != pri) continue;

		Logger::Log("beacon",
			"DropCarriedBeacon: pawn=0x%p pri=0x%p was carrier for mgr=0x%p — clearing slot 11 + CheckBeacon\n",
			Pawn, pri, mgr);

		// Inventory device removal first — UC TgDevice.uc:677 invokes
		// CheckBeacon on inventory change, but we follow up with an explicit
		// call to guarantee the respawn fires even if the inventory hook
		// path doesn't trigger during the Dying / Destroyed teardown.
		ATgInventoryManager* invMgr = (ATgInventoryManager*)Pawn->InvManager;
		if (invMgr) {
			TgInventoryManager__NonPersistRemoveDevice::Call(invMgr, nullptr, 11);
		}

		// Whether or not the inventory remove succeeded, we still need
		// CheckBeacon to re-evaluate. bAttemptRespawn=true so it spawns
		// at the original-priority factory if nobody else is carrying.
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
