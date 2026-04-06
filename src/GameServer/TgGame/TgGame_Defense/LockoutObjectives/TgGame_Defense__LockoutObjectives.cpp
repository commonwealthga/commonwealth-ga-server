#include "src/GameServer/TgGame/TgGame_Defense/LockoutObjectives/TgGame_Defense__LockoutObjectives.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__LockoutObjectives::Call(ATgGame_Defense* Game, void* edx, int nPriority) {
	LogCallBegin();
	CallOriginal(Game, edx, nPriority);
	LogCallEnd();
}
