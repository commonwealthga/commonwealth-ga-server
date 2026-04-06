#include "src/GameServer/TgGame/TgGame/DbSaveReward/TgGame__DbSaveReward.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__DbSaveReward::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
