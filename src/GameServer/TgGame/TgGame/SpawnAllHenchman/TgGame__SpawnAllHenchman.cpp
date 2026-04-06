#include "src/GameServer/TgGame/TgGame/SpawnAllHenchman/TgGame__SpawnAllHenchman.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__SpawnAllHenchman::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
