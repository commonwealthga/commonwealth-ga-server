#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"

#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
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
