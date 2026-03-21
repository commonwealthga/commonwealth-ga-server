#include "src/GameServer/TgGame/TgGame/GetFinalObjectivesList/TgGame__GetFinalObjectivesList.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Returns a pointer to GRI's m_MissionObjectives as the final objectives list.
// For PointRotation, all objectives are candidates; the Arena RoundInProgress
// short-circuits before using this, so it's primarily a safety fallback.
TArray<ATgMissionObjective*>* __fastcall TgGame__GetFinalObjectivesList::Call(ATgGame* Game, void* edx) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) {
		LogCallEnd();
		return nullptr;
	}

	LogCallEnd();
	return &GRI->m_MissionObjectives;
}


