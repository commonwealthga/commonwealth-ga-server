#include "src/GameServer/TgGame/TgGame_Arena/AdjustBeaconForwardSpawn/TgGame_Arena__AdjustBeaconForwardSpawn.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Arena__AdjustBeaconForwardSpawn::Call(ATgGame_Arena* Game, void* edx, int nPriority) {
	LogCallBegin();
	CallOriginal(Game, edx, nPriority);
	LogCallEnd();
}
