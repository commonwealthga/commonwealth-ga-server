#include "src/GameServer/TgGame/TgGame_Control/CalcDefenderReviveTime/TgGame_Control__CalcDefenderReviveTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

float __fastcall TgGame_Control__CalcDefenderReviveTime::Call(ATgGame_Control* Game, void* edx) {
	LogCallBegin();
	float result = CallOriginal(Game, edx);
	LogCallEnd();
	return result;
}
