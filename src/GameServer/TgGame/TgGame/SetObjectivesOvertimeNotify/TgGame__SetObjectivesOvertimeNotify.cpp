#include "src/GameServer/TgGame/TgGame/SetObjectivesOvertimeNotify/TgGame__SetObjectivesOvertimeNotify.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called when entering overtime (m_eTimerState == 3). The GRI's r_bInOverTime flag
// is already set by ChangeTimerState before this is called. Objectives already
// check m_eTimerState at runtime in UpdateObjectiveStatus, so the main purpose here
// is to force a replication update on all active objectives so clients are aware of
// the overtime state transition immediately.
void __fastcall TgGame__SetObjectivesOvertimeNotify::Call(ATgGame* Game, void* edx) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr) {
		LogCallEnd();
		return;
	}

	Logger::Log("objective", "SendObjectivesOvertimeNotify: notifying %d objectives\n",
		GRI->m_MissionObjectives.Count);

	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr && Obj->r_bIsActive) {
			// Force replication so clients see the overtime state immediately
			Obj->bNetDirty = 1;
			Obj->bForceNetUpdate = 1;
		}
	}

	LogCallEnd();
}


