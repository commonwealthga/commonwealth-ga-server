#include "src/GameServer/TgGame/TgGame/AdjustBeaconForwardSpawn/TgGame__AdjustBeaconForwardSpawn.hpp"
#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <unordered_set>

// Stripped native (TgGame__AdjustBeaconForwardSpawn_notimplemented @ 0x10ad9bf0).
// Called when the team's forward-spawn priority advances (we wire this in via
// TgGame::UnlockObjective once s_nCurrentPriority is updated; UC never calls it).
//
// Semantics:
//   * Exit beacon — if anchored to a factory at a different priority than the
//     new one, destroy it. CheckBeacon → SpawnNewBeaconForTeam picks the new
//     priority's factory. A beacon currently carried by a player is unaffected
//     (mgr->r_Beacon == null because PickUpDeployable already UnRegister'd it).
//   * Entrance pads — destroy entrances belonging to a wrong-priority factory,
//     and SpawnObject any matching-priority factory that doesn't yet have a
//     live entrance. SpawnObject's own priority filter handles the spawn-side
//     gate; this routine handles the cleanup-and-reissue dance per-tier.

namespace {

void RelocateTeamBeacon(ATgTeamBeaconManager* mgr, int nPriority) {
	if (!mgr || !mgr->r_Beacon) return;

	ATgDeploy_Beacon* beacon = mgr->r_Beacon;
	ATgBeaconFactory* factory = (ATgBeaconFactory*)beacon->s_DeployFactory;
	const int currFactoryPrio = factory ? factory->m_nPriority : -2;

	// Null s_DeployFactory = player-deployed (we don't anchor those to a
	// factory because the spawn-room factory has s_bAutoSpawn=1 which would
	// drive HUD status to AT_SPAWN (status 5, no HUD case). Treat as
	// forward-deployed: always destroy on tier change so the team's forward
	// spawn re-anchors at the new priority's factory.
	if (factory == nullptr) {
		Logger::Log("beacon",
			"  exit on manager 0x%p is player-deployed (null factory) — destroying for tier change to %d\n",
			mgr, nPriority);
		// fallthrough to destroy below
	}
	else if (currFactoryPrio <= 0 || currFactoryPrio == nPriority) {
		Logger::Log("beacon",
			"  exit on manager 0x%p stays put (factory prio=%d new=%d)\n",
			mgr, currFactoryPrio, nPriority);
		return;
	}

	Logger::Log("beacon",
		"  destroying beacon 0x%p (factory=0x%p prio=%d) on manager 0x%p — new prio=%d\n",
		beacon, factory, currFactoryPrio, mgr, nPriority);

	// eventDestroyIt -> UC TgDeploy_Beacon.DestroyIt -> super.DestroyIt +
	// UnRegisterBeacon, which itself calls vtable[0x37c] = CheckBeacon(true)
	// internally. CheckBeacon increments r_BeaconDestroyed (-> "beacon
	// destroyed" client announcement) when old r_BeaconStatus was nonzero
	// AND s_bAutoAdjustingBeacon is false. We set the flag for the duration
	// of the destroy to suppress the announcement on priority advancement —
	// the beacon isn't lost, the front line just moved. Same idiom UC's
	// InitFor uses (TgTeamBeaconManager.uc:88-91). Our explicit follow-up
	// CheckBeacon kept for the case where UnRegister doesn't match the
	// beacon (defensive — its own CheckBeacon should already have run).
	beacon->s_bWasPickedUp = 0;  // not a pickup — destruction
	const bool wasAutoAdjusting =
		(*reinterpret_cast<unsigned int*>(reinterpret_cast<char*>(mgr) + 0x1EC) & 0x2u) != 0;
	if (!wasAutoAdjusting) {
		*reinterpret_cast<unsigned int*>(reinterpret_cast<char*>(mgr) + 0x1EC) |= 0x2u;
	}
	beacon->eventDestroyIt(1);
	BeaconSdk::CheckBeacon(mgr, true);
	if (!wasAutoAdjusting) {
		*reinterpret_cast<unsigned int*>(reinterpret_cast<char*>(mgr) + 0x1EC) &= ~0x2u;
	}
}

void RelocateTeamEntrances(ATgTeamBeaconManager* mgr, int nPriority) {
	if (!mgr) return;

	UClass* entranceCls = ClassPreloader::GetTgDeployBeaconEntranceClass();
	if (!entranceCls) return;

	// Pass 1: walk live entrances. Destroy any whose factory is at a wrong
	// priority. Record the factories that still have a live entrance so we
	// don't re-spawn a duplicate in pass 2.
	std::unordered_set<ATgBeaconFactory*> factoriesWithLiveEntrance;
	if (UObject::GObjObjects()) {
		const int count = UObject::GObjObjects()->Count;
		for (int i = 0; i < count; ++i) {
			UObject* obj = UObject::GObjObjects()->Data[i];
			if (!obj || obj->Class != entranceCls) continue;
			ATgDeploy_BeaconEntrance* e = (ATgDeploy_BeaconEntrance*)obj;
			if (e->m_bInDestroyedState) continue;
			ATgBeaconFactory* f = (ATgBeaconFactory*)e->s_DeployFactory;
			if (!f) continue;
			// Only manage entrances belonging to THIS team — entrances for
			// other taskforces are handled by their own manager's pass.
			if (mgr->r_TaskForce &&
				(int)f->s_nTaskForce != mgr->r_TaskForce->r_nTaskForce) {
				continue;
			}
			const int prio = f->m_nPriority;
			const bool wantsIt = (prio <= 0 || prio == nPriority);
			if (!wantsIt) {
				Logger::Log("beacon",
					"  destroying entrance 0x%p (factory=0x%p prio=%d) tf=%d — new prio=%d\n",
					e, f, prio, (int)f->s_nTaskForce, nPriority);
				e->eventDestroyIt(1);
			} else {
				factoriesWithLiveEntrance.insert(f);
			}
		}
	}

	// Pass 2: walk the manager's factory list and spawn entrances for
	// matching-priority factories that don't yet have one. SpawnObject's
	// own priority gate is a safety net — if currentPriority has already
	// been updated (it has, by UnlockObjective's pre-call write), spawns
	// land cleanly.
	const int count = mgr->s_BeaconFactoryList.Num();
	for (int i = 0; i < count; ++i) {
		ATgBeaconFactory* f = mgr->s_BeaconFactoryList.Data[i];
		if (!f || f->m_bBeaconExit) continue;
		const int prio = f->m_nPriority;
		const bool wantsIt = (prio <= 0 || prio == nPriority);
		if (!wantsIt) continue;
		if (factoriesWithLiveEntrance.count(f)) continue;

		Logger::Log("beacon",
			"  spawning entrance for factory 0x%p (prio=%d) tf=%d — new prio=%d\n",
			f, prio, (int)f->s_nTaskForce, nPriority);
		TgBeaconFactory__SpawnObject::Call(f, nullptr);
	}
}

void AdjustForTeam(ATgTeamBeaconManager* mgr, int nPriority) {
	if (!mgr) return;
	// Refresh the factory list — entrance factories may have been added/removed
	// since the last InitFor pass (e.g. dynamic Kismet spawns). Cheap and
	// idempotent. Also covers the case where init order populated the list
	// before our s_nTaskForce overrides applied (legacy of the same root
	// issue we patched in SpawnObject).
	BeaconSdk::PopulateBeaconFactoryList(mgr);
	RelocateTeamBeacon(mgr, nPriority);
	RelocateTeamEntrances(mgr, nPriority);
}

}  // namespace

void __fastcall TgGame__AdjustBeaconForwardSpawn::Call(ATgGame* Game, void* /*edx*/, int nPriority) {
	if (!Game) return;

	Logger::Log("beacon",
		"TgGame::AdjustBeaconForwardSpawn nPriority=%d (s_nCurrentPriority=%d s_nPrevPriority=%d)\n",
		nPriority, Game->s_nCurrentPriority, Game->s_nPrevPriority);

	if (GTeamsData.Attackers && GTeamsData.Attackers->r_BeaconManager) {
		AdjustForTeam(GTeamsData.Attackers->r_BeaconManager, nPriority);
	}
	if (GTeamsData.Defenders && GTeamsData.Defenders->r_BeaconManager) {
		AdjustForTeam(GTeamsData.Defenders->r_BeaconManager, nPriority);
	}
}
