#include "src/GameServer/TgGame/TgBotFactory/ResetQueue/TgBotFactory__ResetQueue.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UC signature: native function ResetQueue(optional int nOverrideSpawnTableId = 0);
//
// Called from UC PostBeginPlay (no override) and OnToggle (no override). The
// override is for runtime mode switches where the same factory should start
// drawing from a different table (e.g. wave-system progression).
//
// Behaviour:
//   - If nOverrideSpawnTableId > 0 and differs from nSpawnTableId, switch
//     tables and rebuild the queue + group state from the new table.
//   - Otherwise just reset the runtime cursors. The queue itself was already
//     built by LoadObjectConfig and doesn't need re-emission.
void __fastcall TgBotFactory__ResetQueue::Call(ATgBotFactory* BotFactory, void* edx, int nOverrideSpawnTableId) {
	if (BotFactory == nullptr) return;

	Logger::Log("tgbotfactory",
		"[%s] %s ResetQueue(override=%d) mapObjectId=%d current=%d/%d\n",
		Logger::GetTime(), BotFactory->GetName(), nOverrideSpawnTableId,
		BotFactory->m_nMapObjectId, BotFactory->nCurrentCount, BotFactory->nActiveCount);

	if (nOverrideSpawnTableId > 0 && nOverrideSpawnTableId != BotFactory->nSpawnTableId) {
		BotFactory->nSpawnTableId = nOverrideSpawnTableId;
		BotFactory->m_SpawnQueue.Clear();
		const auto q = TgBotFactory__LoadObjectConfig::CreateRandomSpawnQueue(nOverrideSpawnTableId);
		for (const auto& e : q) BotFactory->m_SpawnQueue.Add(e);

		// Per-group state needs rebuilding too. Same defaults as
		// LoadObjectConfig::PopulateSpawnGroups: leave nMin/nMax at 0 so
		// SpawnNextBot writes them from the picked row's bot_count on first
		// process of each group.
		BotFactory->m_SpawnGroups.Clear();
		const int respawnSecs = static_cast<int>(BotFactory->fRespawnDelay > 0.0f ? BotFactory->fRespawnDelay : 0.0f);
		for (int i = 0; i < BotFactory->m_SpawnQueue.Num(); i++) {
			FSpawnGroupDetail gd;
			gd.nMinCount       = 0;
			gd.nMaxCount       = 0;
			gd.nCurrentCount   = 0;
			gd.nRespawnSeconds = respawnSecs;
			BotFactory->m_SpawnGroups.Add(gd);
		}
	}

	// Cursors + factory-wide counter back to start.
	BotFactory->s_nCurListIndex     = -1;
	BotFactory->s_nCurLocationIndex = -1;
	BotFactory->nCurrentCount       = 0;
	BotFactory->m_nLastGroup        = 0;

	// Per-group nCurrentCount back to 0 — stale counts from a previous round
	// would otherwise wedge the CanSpawnInGroup cap check.
	for (int i = 0; i < BotFactory->m_SpawnGroups.Num(); i++) {
		BotFactory->m_SpawnGroups.Data[i].nCurrentCount = 0;
	}
}
