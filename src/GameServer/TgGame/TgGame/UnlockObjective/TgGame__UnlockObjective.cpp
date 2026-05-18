#include "src/GameServer/TgGame/TgGame/UnlockObjective/TgGame__UnlockObjective.hpp"
#include "src/GameServer/TgGame/TgGame/AdjustBeaconForwardSpawn/TgGame__AdjustBeaconForwardSpawn.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called with nPriority=1 at PostBeginPlay and from TgGame_Arena::ObjectiveUnlock.
// Unlocks and activates all objectives matching the given priority.
// Delegates to eventUnlockObjective which handles: r_bIsLocked, SetObjectiveActive,
// ForceNetRelevant, and TriggerEventClass(TgSeqEvent_ObjectiveStatusChanged, 4).
//
// Skips objectives that are flagged `m_bCaptureOnlyOnce` and have already been
// captured once (`r_bHasBeenCapturedOnce`). Without this gate, the UC path
// in TgMissionObjective::UpdateObjectiveStatus:522-525 re-unlocks the previous
// objective on a status-7 transition (defender pushed attacker capture back
// to zero on the CURRENT point), which incorrectly reactivates already-finished
// permanent objectives in TgGame_Mission-style sequential capture missions.
void __fastcall TgGame__UnlockObjective::Call(ATgGame* Game, void* edx, int nPriority) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) {
		LogCallEnd();
		return;
	}

	// Pre-scan for eligibility BEFORE any state mutation. We need to detect:
	//   (a) nPriority <= 0 — defender-pushback bogus call from UC
	//       UpdateObjectiveStatus:524 (`UnlockObjective(nPriority - 1)`).
	//       Status-7 fires when defenders push attacker capture back to zero
	//       on the CURRENT point — leaving mid-capture triggers it. For the
	//       first objective this becomes UnlockObjective(0). Priority 0 isn't
	//       a real game state (no priority-0 objectives, no priority-0 beacon
	//       factories); letting ABFS run with it destroys every entrance/exit
	//       on the map and there's nothing to respawn.
	//   (b) all matching objectives are capture-only-once + already captured.
	// Either case is a no-op — must not roll s_nCurrentPriority back or trip
	// ABFS.
	if (nPriority <= 0) {
		Logger::Log(GetLogChannel(),
			"UnlockObjective(%d): non-positive priority — bailing (defender-pushback status-7 path)\n",
			nPriority);
		LogCallEnd();
		return;
	}
	int matched = 0, eligible = 0;
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj == nullptr || Obj->nPriority != nPriority) continue;
		matched++;
		if (!(Obj->m_bCaptureOnlyOnce && Obj->r_bHasBeenCapturedOnce)) eligible++;
	}
	if (matched > 0 && eligible == 0) {
		Logger::Log(GetLogChannel(),
			"UnlockObjective(%d): all %d matching objectives are capture-only-once + captured — bailing (no priority mutation, no ABFS)\n",
			nPriority, matched);
		LogCallEnd();
		return;
	}

	Game->s_nPrevPriority = Game->s_nCurrentPriority;
	Game->s_nCurrentPriority = nPriority;

	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj == nullptr || Obj->nPriority != nPriority) continue;

		if (Obj->m_bCaptureOnlyOnce && Obj->r_bHasBeenCapturedOnce) {
			Logger::Log(GetLogChannel(),
				"Skipping unlock of %s — already captured + flagged capture-only-once\n",
				Obj->GetFullName());
			continue;
		}

		Logger::Log(GetLogChannel(), "Unlocking objective %s\n", Obj->GetFullName());
		Obj->eventUnlockObjective(1);  // bActivateObjective=true
	}

	// Drive forward-spawn lifecycle: destroys stale-tier exit/entrance beacons
	// and spawns fresh ones at the new priority. UC never calls this native
	// (it's only declared, never invoked) and the original ATgGame native is
	// stripped — wiring it from here is the single source of truth for
	// priority-advancement beacon updates. Skipped at the bootstrap call
	// (prev priority was 0 — no factories have spawned anything yet);
	// SpawnObject's own priority filter handles the initial state.
	if (Game->s_nPrevPriority > 0 && Game->s_nPrevPriority != nPriority) {
		TgGame__AdjustBeaconForwardSpawn::Call(Game, nullptr, nPriority);
	}

	LogCallEnd();
}


