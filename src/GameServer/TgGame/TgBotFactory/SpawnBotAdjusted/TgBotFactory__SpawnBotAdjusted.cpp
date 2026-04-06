#include "src/GameServer/TgGame/TgBotFactory/SpawnBotAdjusted/TgBotFactory__SpawnBotAdjusted.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgBotFactory__SpawnBotAdjusted::Call(ATgBotFactory* Factory, void* edx, int nBotId, float fAdjustment) {
	LogCallBegin();
	CallOriginal(Factory, edx, nBotId, fAdjustment);
	LogCallEnd();
}
