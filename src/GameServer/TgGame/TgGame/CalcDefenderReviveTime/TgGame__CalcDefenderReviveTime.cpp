#include "src/GameServer/TgGame/TgGame/CalcDefenderReviveTime/TgGame__CalcDefenderReviveTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

float __fastcall TgGame__CalcDefenderReviveTime::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	float result = CallOriginal(Game, edx);
	LogCallEnd();
	return result;
}
