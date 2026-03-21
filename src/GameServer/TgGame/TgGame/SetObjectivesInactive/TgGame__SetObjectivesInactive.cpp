#include "src/GameServer/TgGame/TgGame/SetObjectivesInactive/TgGame__SetObjectivesInactive.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Deactivates all mission objectives without locking them.
void __fastcall TgGame__SetObjectivesInactive::Call(ATgGame* Game, void* edx) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) {
		LogCallEnd();
		return;
	}

	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr && Obj->r_bIsActive) {
			Obj->r_bIsActive = 0;
			Obj->bNetDirty = 1;
			Obj->bForceNetUpdate = 1;
			Obj->OnActiveStateChanged();
		}
	}

	LogCallEnd();
}


