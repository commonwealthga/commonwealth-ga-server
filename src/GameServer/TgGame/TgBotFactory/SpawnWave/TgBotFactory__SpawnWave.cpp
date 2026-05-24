#include "src/GameServer/TgGame/TgBotFactory/SpawnWave/TgBotFactory__SpawnWave.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstdlib>

// SpawnWave(Num) — called by TgBotEncounterVolume.uc:82 when human player
// count rises ("add Num more bots beyond the steady-state"). Does NOT call
// SpawnNextBot because SpawnNextBot cascades through the whole queue on its
// own (PostBeginPlay relies on that to populate the initial roster); calling
// SpawnNextBot Num times here would multiply-cascade. Instead, do a
// flattened Num-iteration spawn that walks the queue + the spawn table
// directly.
void __fastcall TgBotFactory__SpawnWave::Call(ATgBotFactory* BotFactory, void* edx, int Num) {
	if (BotFactory == nullptr || Num <= 0) return;

	Logger::Log("tgbotfactory",
		"[%s] %s SpawnWave Num=%d mapObjectId=%d (current=%d/%d)\n",
		Logger::GetTime(), BotFactory->GetName(), Num, BotFactory->m_nMapObjectId,
		BotFactory->nCurrentCount, BotFactory->nActiveCount);

	if (BotFactory->m_SpawnQueue.Num() == 0 || BotFactory->LocationList.Num() == 0) {
		Logger::Log("tgbotfactory", "  SpawnWave: empty queue or LocationList — nothing to spawn\n");
		return;
	}
	if (BotFactory->m_SpawnQueue.Data == nullptr || BotFactory->LocationList.Data == nullptr) {
		Logger::Log("tgbotfactory", "  SpawnWave: null Data pointer on queue/locations — skipping\n");
		return;
	}

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	int spawned = 0;

	for (int i = 0; i < Num; i++) {
		// Factory-wide cap. 0 = unlimited.
		if (BotFactory->nActiveCount > 0 && BotFactory->nCurrentCount >= BotFactory->nActiveCount) {
			Logger::Log("tgbotfactory",
				"  SpawnWave: hit active cap (%d/%d) after %d of %d — stopping\n",
				BotFactory->nCurrentCount, BotFactory->nActiveCount, spawned, Num);
			break;
		}

		// Advance the queue cursor in step with the requested wave count;
		// wrap as needed.
		BotFactory->s_nCurListIndex = (BotFactory->s_nCurListIndex + 1) % BotFactory->m_SpawnQueue.Num();
		BotFactory->s_nCurLocationIndex = (BotFactory->s_nCurLocationIndex + 1) % BotFactory->LocationList.Num();

		FSpawnQueueEntry* entry = &BotFactory->m_SpawnQueue.Data[BotFactory->s_nCurListIndex];
		ANavigationPoint* BotStart = BotFactory->LocationList.Data[BotFactory->s_nCurLocationIndex];
		if (BotStart == nullptr) continue;

		int dummyCount = 1;
		const int botId = TgBotFactory__LoadObjectConfig::PickBotFromSpawnTableGroup(
			entry->nSpawnTableId, entry->nGroupNumber, &dummyCount);
		if (botId <= 0) continue;

		FVector loc = BotStart->Location;
		{
			float radius = 0.0f, halfHeight = 0.0f;
			TgGame__SpawnBotById::GetBotCollisionCylinder(botId, &radius, &halfHeight);
			if (halfHeight > 0.0f) loc.Z += halfHeight + 5.0f;
		}

		ATgPawn* Bot = (ATgPawn*)Game->SpawnBotById(
			botId, loc, BotStart->Rotation,
			false, BotFactory, true, nullptr, false, nullptr, 0.0f);
		if (Bot == nullptr || Bot->Controller == nullptr) continue;

		/*
		ATgAIController* AIController = (ATgAIController*)Bot->Controller;
		for (int j = 0; j < BotFactory->PatrolPath.Num(); j++) {
			AIController->m_PatrolPath.Add(BotFactory->PatrolPath.Data[j]);
		}
		if (AIController->m_PatrolPath.Num() == 0) {
			AIController->m_PatrolPath.Add(BotStart);
		}*/

		ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
		if (BotRep != nullptr) {
			const int team = (BotFactory->s_nTaskForce == 1) ? GTeamsData.Attackers : GTeamsData.Defenders;
			BotRep->r_TaskForce = team;
			BotRep->Team        = team;
			BotRep->SetTeam(team);
			Bot->NotifyTeamChanged();
		}

		BotFactory->nCurrentCount += 1;
		BotFactory->nTotalSpawns  += 1;
		BotFactory->m_nSpawnOrder += 1;
		BotFactory->m_nLastGroup   = entry->nGroupNumber;
		const int gi = BotFactory->s_nCurListIndex;
		if (gi >= 0 && gi < BotFactory->m_SpawnGroups.Num()) {
			BotFactory->m_SpawnGroups.Data[gi].nCurrentCount += 1;
		}
		spawned++;
	}

	Logger::Log("tgbotfactory",
		"  SpawnWave complete: spawned %d/%d (current=%d/%d)\n",
		spawned, Num, BotFactory->nCurrentCount, BotFactory->nActiveCount);
}
