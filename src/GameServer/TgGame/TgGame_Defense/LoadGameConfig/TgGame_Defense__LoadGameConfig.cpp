#include "src/GameServer/TgGame/TgGame_Defense/LoadGameConfig/TgGame_Defense__LoadGameConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__LoadGameConfig::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
