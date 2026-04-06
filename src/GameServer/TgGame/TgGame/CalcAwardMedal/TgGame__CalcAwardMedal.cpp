#include "src/GameServer/TgGame/TgGame/CalcAwardMedal/TgGame__CalcAwardMedal.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__CalcAwardMedal::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
