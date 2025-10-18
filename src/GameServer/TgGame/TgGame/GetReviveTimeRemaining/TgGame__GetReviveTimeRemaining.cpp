#include "src/GameServer/TgGame/TgGame/GetReviveTimeRemaining/TgGame__GetReviveTimeRemaining.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

int __fastcall TgGame__GetReviveTimeRemaining::Call(ATgGame* Game, void* edx, AController* Controller) {
	AWorldInfo* WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;

	Game->ReviveAttackersTimer();

	return 1;

	// int result = (int)WorldInfo->TimeSeconds % (int)Game->m_nSecsToAutoRelease;
	//
	// Logger::Log("debug", "TgGame::GetReviveTimeRemaining: %d\n", result);
	//
	// Game->SetTimer(result, 0, FName("ReviveAttackersTimer"), nullptr);
	//
	// return result;
}

