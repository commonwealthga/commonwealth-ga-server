#include "src/GameServer/TgGame/TgGame/LockoutObjectives/TgGame__LockoutObjectives.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called with nPriority=0 from ResetObjectives (locks/deactivates all objectives).
// When nPriority==0, locks and deactivates all objectives.
// When nPriority>0, locks objectives at that priority and below (used for progression gating).
// Delegates to eventLockObjective which handles: r_bIsLocked, SetObjectiveActive,
// ForceNetRelevant, and TriggerEventClass(TgSeqEvent_ObjectiveStatusChanged, 5).
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
			Obj->eventLockObjective();
		}
	}

	LogCallEnd();
}


