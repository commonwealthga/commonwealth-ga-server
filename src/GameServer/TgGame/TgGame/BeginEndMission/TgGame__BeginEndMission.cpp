#include "src/GameServer/TgGame/TgGame/BeginEndMission/TgGame__BeginEndMission.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgGame__BeginEndMission::Call(ATgGame* Game, void* edx, unsigned long bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride) {
	LogCallBegin();
	bool result = CallOriginal(Game, edx, bClearNextMapGame, endMissionCamera, fDelayOverride);
	LogCallEnd();
	return result;
}
