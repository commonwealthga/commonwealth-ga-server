#include "src/GameServer/TgGame/TgMissionObjective/SetObjectiveActive/TgMissionObjective__SetObjectiveActive.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgMissionObjective__SetObjectiveActive::Call(ATgMissionObjective* Obj, void* edx, unsigned long bActive) {
	LogCallBegin();

	Obj->r_bIsActive = bActive ? 1 : 0;
	Obj->bNetDirty = 1;
	Obj->bForceNetUpdate = 1;

	Obj->OnActiveStateChanged();

	LogCallEnd();
}


