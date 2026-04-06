#include "src/GameServer/TgGame/TgGame/AdjustBeaconForwardSpawn/TgGame__AdjustBeaconForwardSpawn.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__AdjustBeaconForwardSpawn::Call(ATgGame* Game, void* edx, int nPriority) {
	LogCallBegin();
	CallOriginal(Game, edx, nPriority);
	LogCallEnd();
}
