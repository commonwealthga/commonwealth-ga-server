#include "src/GameServer/TgGame/TgGame/NotifyPostCommitMapChange/TgGame__NotifyPostCommitMapChange.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__NotifyPostCommitMapChange::Call(ATgGame* Game, void* edx) {
	LogCallBegin();
	CallOriginal(Game, edx);
	LogCallEnd();
}
