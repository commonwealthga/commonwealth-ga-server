#include "src/GameServer/TgGame/TgGame/UnlockObjective/TgGame__UnlockObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called with nPriority=1 at PostBeginPlay and from TgGame_Arena::ObjectiveUnlock.
// Unlocks and activates all objectives matching the given priority.
// Delegates to eventUnlockObjective which handles: r_bIsLocked, SetObjectiveActive,
// ForceNetRelevant, and TriggerEventClass(TgSeqEvent_ObjectiveStatusChanged, 4).
void __fastcall TgGame__UnlockObjective::Call(ATgGame* Game, void* edx, int nPriority) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) {
		LogCallEnd();
		return;
	}

	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr && Obj->nPriority == nPriority) {
			Logger::Log(GetLogChannel(), "Unlocking objective %s\n", Obj->GetFullName());
			Obj->eventUnlockObjective(1);  // bActivateObjective=true
		}
	}

	LogCallEnd();
}


