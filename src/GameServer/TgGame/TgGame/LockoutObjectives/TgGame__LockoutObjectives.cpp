#include "src/GameServer/TgGame/TgGame/LockoutObjectives/TgGame__LockoutObjectives.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Two callers in UC:
//   1. TgGame_Arena.ResetObjectives() → LockoutObjectives(0) — fresh-game reset,
//      lock every objective regardless of state.
//   2. TgMissionObjective.UpdateObjectiveStatus → Game.LockoutObjectives(nPriority)
//      whenever IsContested() becomes true. nPriority is the priority of the
//      objective whose UpdateObjectiveStatus just fired. The intent is to lock
//      sibling priority-N objectives so they can't be captured in parallel —
//      NOT to lock the contested objective itself.
//
// At call-time in path (2), the contested objective already has r_eStatus set
// to the new value (2..6 — PAUSED_CONTESTED / *_INHOLD / *_INPROGRESS), while
// untouched siblings are still at their default r_eStatus=7 (DEFENDER_CAPTURED).
// Skip status 2..6 so the contested objective stays unlocked — otherwise
// eventLockObjective sets r_bIsLocked + r_bIsActive=false and the next
// TgMissionObjective_Proximity.Tick early-returns on
// `if(r_bIsLocked || !r_bIsActive) return;`, freezing capture progression.
//
// Reset path (nPriority==0) still locks everything — at reset every objective
// is at default status 7 anyway, so the filter wouldn't trip there, but we
// bypass the check explicitly to make the reset semantics unambiguous.
void __fastcall TgGame__LockoutObjectives::Call(ATgGame* Game, void* edx, int nPriority) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) {
		LogCallEnd();
		return;
	}

	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj == nullptr) continue;

		if (nPriority == 0 || Obj->nPriority == nPriority) {
			if (nPriority != 0 && Obj->r_eStatus >= 2 && Obj->r_eStatus <= 6) continue;
			Obj->eventLockObjective();
		}
	}

	LogCallEnd();
}


