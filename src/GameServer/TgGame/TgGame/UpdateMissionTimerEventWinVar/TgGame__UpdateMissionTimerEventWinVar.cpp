#include "src/GameServer/TgGame/TgGame/UpdateMissionTimerEventWinVar/TgGame__UpdateMissionTimerEventWinVar.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__UpdateMissionTimerEventWinVar::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
