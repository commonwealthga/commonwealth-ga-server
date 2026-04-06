#include "src/GameServer/TgGame/TgGame/FinishEndMission/TgGame__FinishEndMission.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgGame__FinishEndMission::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	bool result = CallOriginal(Game, edx);
	LogCallEnd();
	return result;
}
