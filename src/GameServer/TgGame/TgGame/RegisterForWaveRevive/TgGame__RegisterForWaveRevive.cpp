#include "src/GameServer/TgGame/TgGame/RegisterForWaveRevive/TgGame__RegisterForWaveRevive.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

void __fastcall TgGame__RegisterForWaveRevive::Call(ATgGame *Game, void *edx, AController *Controller) {

	Logger::Log("debug", "TgGame::RegisterForWaveRevive called\n");

	TARRAY_INIT(Game, AttackerReviveList, AController*, 0x418, 32);
	TARRAY_INIT(Game, DefenderReviveList, AController*, 0x424, 32);

	TARRAY_ADD(AttackerReviveList, Controller);
}

