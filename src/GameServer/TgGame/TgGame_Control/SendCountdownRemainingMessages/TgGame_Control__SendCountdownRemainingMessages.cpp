#include "src/GameServer/TgGame/TgGame_Control/SendCountdownRemainingMessages/TgGame_Control__SendCountdownRemainingMessages.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Control__SendCountdownRemainingMessages::Call(ATgGame_Control* Game, void* edx, float oldTimeRemaining, float newTimeRemaining) {
	LogCallBegin();
	CallOriginal(Game, edx, oldTimeRemaining, newTimeRemaining);
	LogCallEnd();
}
