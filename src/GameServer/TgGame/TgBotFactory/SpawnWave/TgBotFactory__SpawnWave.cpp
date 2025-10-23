#include "src/GameServer/TgGame/TgBotFactory/SpawnWave/TgBotFactory__SpawnWave.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgBotFactory__SpawnWave::Call(ATgBotFactory *BotFactory, void *edx, int Num) {
	Logger::Log("tgbotfactory", "[%s] %s SpawnWave Num=%d\n", Logger::GetTime(), BotFactory->GetName(), Num);
}

