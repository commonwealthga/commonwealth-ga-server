#include "src/GameServer/TgGame/TgGame_Defense/CheckWinRound/TgGame_Defense__CheckWinRound.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgGame_Defense__CheckWinRound::Call(ATgGame_Defense* Game, void* edx, ATgRepInfo_TaskForce** Winner) {
	LogCallBegin();
	bool result = CallOriginal(Game, edx, Winner);
	LogCallEnd();
	return result;
}
