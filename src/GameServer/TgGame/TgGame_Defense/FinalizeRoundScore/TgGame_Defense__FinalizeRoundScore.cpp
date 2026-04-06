#include "src/GameServer/TgGame/TgGame_Defense/FinalizeRoundScore/TgGame_Defense__FinalizeRoundScore.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__FinalizeRoundScore::Call(ATgGame_Defense* Game, void* edx, ATgRepInfo_TaskForce* Winner) {
	LogCallBegin();
	CallOriginal(Game, edx, Winner);
	LogCallEnd();
}
