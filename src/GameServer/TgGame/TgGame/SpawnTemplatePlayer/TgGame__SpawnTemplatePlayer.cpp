#include "src/GameServer/TgGame/TgGame/SpawnTemplatePlayer/TgGame__SpawnTemplatePlayer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void* __fastcall TgGame__SpawnTemplatePlayer::Call(ATgGame* Game, void* edx, ATgPlayerController* pTgPC, FName sName) {
	LogCallBegin();
	void* result = CallOriginal(Game, edx, pTgPC, sName);
	LogCallEnd();
	return result;
}
