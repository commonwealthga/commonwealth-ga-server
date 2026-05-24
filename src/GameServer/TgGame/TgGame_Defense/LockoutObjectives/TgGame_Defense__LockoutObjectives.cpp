#include "src/GameServer/TgGame/TgGame_Defense/LockoutObjectives/TgGame_Defense__LockoutObjectives.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__LockoutObjectives::Call(ATgGame_Defense* Game, void* edx, int nPriority) {
	LogCallBegin();

	// Defense/Raid does not use the base priority ladder. TgGame_Defense.uc
	// leaves ResetObjectives empty and PostBeginPlay unlocks every objective,
	// so objective captures must not lock sibling objectives through the base
	// TgGame::LockoutObjectives path.
	Logger::Log("gametimer", "LockoutObjectives(%d): ignored for Defense mode\n", nPriority);

	LogCallEnd();
}
