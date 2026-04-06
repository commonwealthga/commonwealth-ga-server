#include "src/GameServer/TgGame/TgGame/GetDifficultyModifier/TgGame__GetDifficultyModifier.hpp"
#include "src/Utils/Logger/Logger.hpp"

float __fastcall TgGame__GetDifficultyModifier::Call(ATgGame* Game, void* edx, int nPriority) {
	LogCallBegin();
	float result = CallOriginal(Game, edx, nPriority);
	LogCallEnd();
	return result;
}
