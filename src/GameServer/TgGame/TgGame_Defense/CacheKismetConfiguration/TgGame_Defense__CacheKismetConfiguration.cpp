#include "src/GameServer/TgGame/TgGame_Defense/CacheKismetConfiguration/TgGame_Defense__CacheKismetConfiguration.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__CacheKismetConfiguration::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
