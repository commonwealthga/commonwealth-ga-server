#include "src/GameServer/TgGame/TgGame/ReviveDefendersTimer/TgGame__ReviveDefendersTimer.hpp"
#include "src/GameServer/TgGame/TgGame/ReviveAttackersTimer/TgGame__ReviveAttackersTimer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame__ReviveDefendersTimer::Call(ATgGame *Game, void *edx) {
	LogCallBegin();
	TgGame__ReviveAttackersTimer::RunWave(Game, false);
	LogCallEnd();
}
