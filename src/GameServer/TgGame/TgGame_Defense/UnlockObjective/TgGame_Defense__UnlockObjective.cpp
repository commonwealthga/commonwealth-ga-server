#include "src/GameServer/TgGame/TgGame_Defense/UnlockObjective/TgGame_Defense__UnlockObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__UnlockObjective::Call(ATgGame_Defense* Game, void* edx, int nPriority) {
	LogCallBegin();
	CallOriginal(Game, edx, nPriority);
	LogCallEnd();
}
