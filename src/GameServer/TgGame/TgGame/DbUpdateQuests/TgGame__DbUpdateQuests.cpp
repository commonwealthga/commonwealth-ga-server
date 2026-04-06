#include "src/GameServer/TgGame/TgGame/DbUpdateQuests/TgGame__DbUpdateQuests.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__DbUpdateQuests::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
