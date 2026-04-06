#include "src/GameServer/TgGame/TgGame/SpawnLandMarks/TgGame__SpawnLandMarks.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__SpawnLandMarks::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
