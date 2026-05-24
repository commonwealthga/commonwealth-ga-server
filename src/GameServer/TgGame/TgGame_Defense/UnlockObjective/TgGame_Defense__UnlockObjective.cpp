#include "src/GameServer/TgGame/TgGame_Defense/UnlockObjective/TgGame_Defense__UnlockObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__UnlockObjective::Call(ATgGame_Defense* Game, void* edx, int nPriority) {
	LogCallBegin();

	if (Game == nullptr || Game->GameReplicationInfo == nullptr) {
		LogCallEnd();
		return;
	}

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	int unlocked = 0;

	for (int i = 0; i < GRI->m_MissionObjectives.Num(); i++) {
		ATgMissionObjective* Objective = GRI->m_MissionObjectives.Data[i];
		if (Objective == nullptr) continue;
		Objective->eventUnlockObjective(1);
		unlocked++;
	}

	Logger::Log("gametimer",
		"UnlockObjective(%d): unlocked all %d registered objectives for Defense mode\n",
		nPriority, unlocked);

	LogCallEnd();
}
