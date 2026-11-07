#include "src/GameServer/TgGame/TgGame/GetFinalObjectivesList/TgGame__GetFinalObjectivesList.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Returns a copy of GRI's m_MissionObjectives as the final objectives list.
// For PointRotation, all objectives are candidates; the Arena RoundInProgress
// short-circuits before using this, so it's primarily a safety fallback.
// The caller owns *Result (uninitialized on entry) and frees it.
TArray<ATgMissionObjective*>* __fastcall TgGame__GetFinalObjectivesList::Call(ATgGame* Game, void* edx, TArray<ATgMissionObjective*>* Result) {
	LogCallBegin();

	Result->Data = nullptr;
	Result->Count = Result->Max = 0;

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI != nullptr) {
		for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
			Result->Add(GRI->m_MissionObjectives.Data[i]);
		}
	}

	LogCallEnd();
	return Result;
}
