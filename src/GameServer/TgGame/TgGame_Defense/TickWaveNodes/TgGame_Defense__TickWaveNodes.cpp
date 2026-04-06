#include "src/GameServer/TgGame/TgGame_Defense/TickWaveNodes/TgGame_Defense__TickWaveNodes.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Defense__TickWaveNodes::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
