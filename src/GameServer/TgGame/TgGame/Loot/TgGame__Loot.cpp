#include "src/GameServer/TgGame/TgGame/Loot/TgGame__Loot.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__Loot::Call(ATgGame* Game, void* edx, int nLootTableId, FVector vBaseLocation, int nTaskForce) {
	LogCallBegin();
	CallOriginal(Game, edx, nLootTableId, vBaseLocation, nTaskForce);
	LogCallEnd();
}
