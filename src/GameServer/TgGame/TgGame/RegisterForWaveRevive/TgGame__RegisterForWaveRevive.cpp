#include "src/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.hpp"
#include "src/SDK/SdkHeaders.h"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

void __fastcall TgGame__RegisterForWaveRevive::Call(ATgGame *Game, void *edx, AController *Controller) {

	Logger::Log("debug", "TgGame::RegisterForWaveRevive called\n");

	TARRAY_INIT(Game, AttackerReviveList, AController*, 0x418, 32);
	TARRAY_INIT(Game, DefenderReviveList, AController*, 0x424, 32);
	
	ATgPlayerController* PlayerController = (ATgPlayerController*)Controller;
	ATgRepInfo_Player* RepInfo = (ATgRepInfo_Player*)PlayerController->PlayerReplicationInfo;
	if (RepInfo->r_TaskForce->IsAttacker()) {
		TARRAY_ADD(AttackerReviveList, Controller);
	} else {
		TARRAY_ADD(DefenderReviveList, Controller);
	}
}

