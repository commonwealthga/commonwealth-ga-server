#include "src/GameServer/TgGame/TgGame/IsFinalObjective/TgGame__IsFinalObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

// For PointRotation, Arena's RoundInProgress.ObjectiveTaken() short-circuits before
// calling this. Returns false for all objectives since round completion is handled
// by FinalizeRoundScore, not the base ObjectiveTaken path.
bool __fastcall TgGame__IsFinalObjective::Call(ATgGame* Game, void* edx, ATgMissionObjective* Objective) {
	LogCallBegin();
	LogCallEnd();
	return false;
}


