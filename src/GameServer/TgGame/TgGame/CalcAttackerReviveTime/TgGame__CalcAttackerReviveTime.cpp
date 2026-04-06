#include "src/GameServer/TgGame/TgGame/CalcAttackerReviveTime/TgGame__CalcAttackerReviveTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

float __fastcall TgGame__CalcAttackerReviveTime::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	float result = CallOriginal(Game, edx);
	LogCallEnd();
	return result;
}
