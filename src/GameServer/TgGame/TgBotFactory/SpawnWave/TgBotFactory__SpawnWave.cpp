#include "src/GameServer/TgGame/TgBotFactory/SpawnWave/TgBotFactory__SpawnWave.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/Utils/Logger/Logger.hpp"

// SpawnWave(Num) — UC caller: TgBotEncounterVolume.uc:82, fired when the
// human player count in an open-world encounter rises ("scale the encounter
// up"). Push Num extra pending-spawn entries distributed round-robin across
// the factory's group indices (due immediately), then kick the drain.
// Distribution heuristic — retail's exact scaling is unrecoverable; tune
// from playtests.
void __fastcall TgBotFactory__SpawnWave::Call(ATgBotFactory* BotFactory, void* edx, int Num) {
	if (BotFactory == nullptr || Num <= 0) return;

	const int nGroups = BotFactory->m_SpawnGroups.Num();
	Logger::Log("tgbotfactory",
		"[%s] %s SpawnWave Num=%d mapObjectId=%d (queue=%d groups=%d current=%d)\n",
		Logger::GetTime(), BotFactory->GetName(), Num, BotFactory->m_nMapObjectId,
		BotFactory->m_SpawnQueue.Num(), nGroups, BotFactory->nCurrentCount);

	if (nGroups <= 0 || BotFactory->nSpawnTableId <= 0) {
		Logger::Log("tgbotfactory", "  SpawnWave: no groups/table — nothing to add\n");
		return;
	}

	for (int k = 0; k < Num; k++) {
		FSpawnQueueEntry entry;
		entry.nGroupNumber  = k % nGroups;  // group INDEX
		entry.fSpawnTime    = 0.0f;
		entry.bRespawn      = 0;
		entry.nSpawnTableId = BotFactory->nSpawnTableId;
		BotFactory->m_SpawnQueue.Add(entry);
	}
	BotFactory->nBotCount += Num;

	TgBotFactory__SpawnNextBot::Call(BotFactory, nullptr);
}
