#include "src/GameServer/TgGame/TgBotFactory/SpawnBotId/TgBotFactory__SpawnBotId.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgBotFactory__SpawnBotId::Call(ATgBotFactory* Factory, void* edx, int nBotId) {
	LogCallBegin();
	Logger::Log("tgbotfactory", "SpawnBotId: %d\n", nBotId);
	CallOriginal(Factory, edx, nBotId);
	LogCallEnd();
}
