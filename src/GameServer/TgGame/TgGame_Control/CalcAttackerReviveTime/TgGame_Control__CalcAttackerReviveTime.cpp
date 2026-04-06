#include "src/GameServer/TgGame/TgGame_Control/CalcAttackerReviveTime/TgGame_Control__CalcAttackerReviveTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

float __fastcall TgGame_Control__CalcAttackerReviveTime::Call(ATgGame_Control* Game, void* edx) {
	LogCallBegin();
	float result = CallOriginal(Game, edx);
	LogCallEnd();
	return result;
}
