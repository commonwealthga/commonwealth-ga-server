#include "src/GameServer/TgGame/TgGame/MissionTimeRemaining/TgGame__MissionTimeRemaining.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"

float __fastcall TgGame__MissionTimeRemaining::Call(ATgGame *Game, void *edx) {
	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	float now = WorldInfo->TimeSeconds;
	int state = (int)Game->m_eTimerState;

	if (state == 1) {
		// Setup phase: s_fMissionTimerStartedAt = mission start time (setup end)
		return Game->s_fMissionTimerStartedAt - now;
	} else if (state == 2) {
		// Mission phase
		return (Game->s_fMissionTimerStartedAt + Game->m_fGameMissionTime) - now;
	} else if (state == 3) {
		// Overtime: starts immediately when mission timer fires
		return (Game->s_fMissionTimerStartedAt + Game->m_fGameMissionTime + Game->m_fGameOvertimeTime) - now;
	}
	return 0.0f;
}

