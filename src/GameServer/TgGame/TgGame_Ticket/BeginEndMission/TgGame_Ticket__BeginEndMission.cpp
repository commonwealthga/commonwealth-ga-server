#include "src/GameServer/TgGame/TgGame_Ticket/BeginEndMission/TgGame_Ticket__BeginEndMission.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgGame_Ticket__BeginEndMission::Call(ATgGame_Ticket* Game, void* edx, unsigned long bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride) {
	LogCallBegin();
	bool result = CallOriginal(Game, edx, bClearNextMapGame, endMissionCamera, fDelayOverride);
	LogCallEnd();
	return result;
}
