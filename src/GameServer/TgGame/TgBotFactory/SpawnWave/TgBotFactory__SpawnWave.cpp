#include "src/GameServer/TgGame/TgBotFactory/SpawnWave/TgBotFactory__SpawnWave.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Spawns a wave of Num bots from this factory's spawn table.
// Called by TgBotEncounterVolume and Kismet wave spawner sequences.
// Resets the queue and spawns Num entries worth of bots via SpawnNextBot.
void __fastcall TgBotFactory__SpawnWave::Call(ATgBotFactory *BotFactory, void *edx, int Num) {
	Logger::Log("tgbotfactory", "[%s] %s SpawnWave Num=%d mapObjectId=%d\n",
		Logger::GetTime(), BotFactory->GetName(), Num, BotFactory->m_nMapObjectId);

	if (Num <= 0) return;

	// Ensure config is loaded
	if (!TgBotFactory__LoadObjectConfig::m_botFactoryConfigs.count(BotFactory->m_nMapObjectId)) {
		Logger::Log("tgbotfactory", "  SpawnWave: no config for mapObjectId=%d\n", BotFactory->m_nMapObjectId);
		return;
	}

	// Reset queue position so SpawnNextBot starts from the beginning
	BotFactory->s_nCurListIndex = -1;
	BotFactory->s_nCurLocationIndex = -1;

	// SpawnNextBot processes one queue entry and recursively calls itself for remaining entries.
	// Call it Num times to spawn Num queue entries worth of bots.
	// If the queue has fewer entries than Num, SpawnNextBot will wrap around.
	for (int i = 0; i < Num; i++) {
		if (BotFactory->m_SpawnQueue.Num() == 0) {
			Logger::Log("tgbotfactory", "  SpawnWave: spawn queue empty, cannot spawn more\n");
			break;
		}
		BotFactory->SpawnNextBot();
	}

	Logger::Log("tgbotfactory", "  SpawnWave complete\n");
}

