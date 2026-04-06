#include "src/GameServer/TgGame/TgGame_Defense/CheckWinGame/TgGame_Defense__CheckWinGame.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgGame_Defense__CheckWinGame::Call(ATgGame_Defense* Game, void* edx, unsigned long bForceWin, ATgRepInfo_TaskForce** Winner) {
	LogCallBegin();
	bool result = CallOriginal(Game, edx, bForceWin, Winner);
	LogCallEnd();
	return result;
}
