#include "src/GameServer/TgGame/TgGame/IsFinalObjective/TgGame__IsFinalObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

// An objective is "final" if it has the highest nPriority among all objectives.
// Used by TgGame::ObjectiveTaken to determine if capturing this objective ends the game.
// For Arena/PointRotation, RoundInProgress.ObjectiveTaken() short-circuits before
// calling the base TgGame::ObjectiveTaken, so this only matters for Mission/Control modes.
bool __fastcall TgGame__IsFinalObjective::Call(ATgGame* Game, void* edx, ATgMissionObjective* Objective) {
	LogCallBegin();

	if (Objective == nullptr) {
		LogCallEnd();
		return false;
	}

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr || GRI->m_MissionObjectives.Count == 0) {
		LogCallEnd();
		return false;
	}

	// Find the highest priority among all objectives
	int maxPriority = 0;
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr && Obj->nPriority > maxPriority) {
			maxPriority = Obj->nPriority;
		}
	}

	bool bIsFinal = (Objective->nPriority == maxPriority);

	Logger::Log("objective", "IsFinalObjective: %s priority=%d maxPriority=%d isFinal=%d\n",
		((UObject*)Objective)->GetName(), Objective->nPriority, maxPriority, bIsFinal);

	LogCallEnd();
	return bIsFinal;
}


