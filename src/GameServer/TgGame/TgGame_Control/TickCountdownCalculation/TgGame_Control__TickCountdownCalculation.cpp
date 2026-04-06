#include "src/GameServer/TgGame/TgGame_Control/TickCountdownCalculation/TgGame_Control__TickCountdownCalculation.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Control__TickCountdownCalculation::Call(ATgGame_Control* Game, void* edx, float DeltaTime) {
	LogCallBegin();
	CallOriginal(Game, edx, DeltaTime);
	LogCallEnd();
}
