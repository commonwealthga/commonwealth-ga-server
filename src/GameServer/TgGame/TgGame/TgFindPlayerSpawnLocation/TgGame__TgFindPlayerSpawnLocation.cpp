#include "src/GameServer/TgGame/TgGame/TgFindPlayerSpawnLocation/TgGame__TgFindPlayerSpawnLocation.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgGame__TgFindPlayerSpawnLocation::Call(ATgGame* Game, void* edx, AController* pPlayer, FVector* vLoc) {
	LogCallBegin();
	bool result = CallOriginal(Game, edx, pPlayer, vLoc);
	LogCallEnd();
	return result;
}
