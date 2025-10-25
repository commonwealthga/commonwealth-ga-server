#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, ATgPawn*> TgBotFactory__SpawnNextBot::m_lastSpawnedBot;

void __fastcall TgBotFactory__SpawnNextBot::Call(ATgBotFactory *BotFactory, void *edx) {

	Logger::Log("tgbotfactory", "[%s] %s SpawnNextBot mapObjectId=%d\n", Logger::GetTime(), BotFactory->GetName(), BotFactory->m_nMapObjectId);

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;

	BotFactory->s_nCurListIndex += 1;
	BotFactory->s_nCurLocationIndex += 1;

	if (BotFactory->s_nCurListIndex >= BotFactory->m_SpawnQueue.Num()) {
		BotFactory->s_nCurListIndex = 0;
	}

	if (BotFactory->s_nCurLocationIndex >= BotFactory->LocationList.Num()) {
		BotFactory->s_nCurLocationIndex = 0;
	}

	if (BotFactory->m_SpawnQueue.Num() == 0) {
		Logger::Log("tgbotfactory", " Spawn queue is empty\n");
		return;
	}

	if (BotFactory->LocationList.Num() == 0) {
		Logger::Log("tgbotfactory", " Location list is empty\n");
		return;
	}

	if (!TgBotFactory__LoadObjectConfig::m_botFactoryConfigs[BotFactory->m_nMapObjectId].SpawnTables.size()) {
		Logger::Log("tgbotfactory", " No config found for map object id %d\n", BotFactory->m_nMapObjectId);
		return;
	}

	BotFactoryConfig config = TgBotFactory__LoadObjectConfig::m_botFactoryConfigs[BotFactory->m_nMapObjectId];

	FSpawnQueueEntry* entry = &BotFactory->m_SpawnQueue.Data[BotFactory->s_nCurListIndex];

	if (config.SpawnTables.size() == 0) {
		Logger::Log("tgbotfactory", " No spawn table found for map object id %d\n", BotFactory->m_nMapObjectId);
		return;
	}

	if (config.SpawnTables[entry->nGroupNumber].size() == 0) {
		Logger::Log("tgbotfactory", " No spawn table found for spawn queue entry %d and group %d\n", BotFactory->s_nCurListIndex, entry->nGroupNumber);
		return;
	}

	std::vector<SpawnTableEntry> spawnTable = config.SpawnTables[entry->nGroupNumber];

	for (auto& randomSpawnEntry : spawnTable) {
		for (int i = 0; i < randomSpawnEntry.BotCount; i++) {
			BotFactory->s_nCurLocationIndex += 1;
			if (BotFactory->s_nCurLocationIndex >= BotFactory->LocationList.Num()) {
				BotFactory->s_nCurLocationIndex = 0;
			}

			Logger::Log("tgbotfactory", " Spawning bot %d %s at X=%d,Y=%d,Z=%d", randomSpawnEntry.EnemyBotId, randomSpawnEntry.ReferenceName.c_str(), BotFactory->LocationList.Data[BotFactory->s_nCurLocationIndex]->Location.X, BotFactory->LocationList.Data[BotFactory->s_nCurLocationIndex]->Location.Y, BotFactory->LocationList.Data[BotFactory->s_nCurLocationIndex]->Location.Z);

			ANavigationPoint* BotStart = BotFactory->LocationList.Data[BotFactory->s_nCurLocationIndex];

			FVector SpawnLocation;
			SpawnLocation.X = BotStart->Location.X;
			SpawnLocation.Y = BotStart->Location.Y;
			SpawnLocation.Z = BotStart->Location.Z;


			ATgPawn* Bot = (ATgPawn*)Game->SpawnBotById(
				randomSpawnEntry.EnemyBotId,
				SpawnLocation,
				BotStart->Rotation,
				false,
				BotFactory,
				true,
				nullptr,
				false,
				nullptr,
				0.0f
			);

			m_lastSpawnedBot[BotFactory->m_nMapObjectId] = Bot;

			ATgAIController* AIController = (ATgAIController*)Bot->Controller;
			for (int j = 0; j < BotFactory->PatrolPath.Num(); j++) {
				AIController->m_PatrolPath.Add(BotFactory->PatrolPath.Data[i]);
			}

			ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;

			if (BotFactory->s_nTaskForce == 1) {
				Logger::Log("tgbotfactory", " as Attacker\n");
				BotRep->r_TaskForce = GTeamsData.Attackers;
				BotRep->Team = GTeamsData.Attackers;
				BotRep->SetTeam(GTeamsData.Attackers);
				Bot->NotifyTeamChanged();
			} else {
				Logger::Log("tgbotfactory", " as Defender\n");
				BotRep->r_TaskForce = GTeamsData.Defenders;
				BotRep->Team = GTeamsData.Defenders;
				BotRep->SetTeam(GTeamsData.Defenders);
				Bot->NotifyTeamChanged();
			}


			Bot->Role = 3;
			Bot->RemoteRole = 1;
			Bot->bNetDirty = 1;
			Bot->bNetInitial = 1;
			Bot->bForceNetUpdate = 1;
			Bot->bSkipActorPropertyReplication = 0;
			Bot->bAlwaysRelevant = 1;

			BotRep->Role = 3;
			BotRep->RemoteRole = 1;
			BotRep->bNetDirty = 1;
			BotRep->bNetInitial = 1;
			BotRep->bForceNetUpdate = 1;
			BotRep->bSkipActorPropertyReplication = 0;
			// BotRep->bAlwaysRelevant = 1;

		}
	}

	if (BotFactory->s_nCurListIndex + 1 < BotFactory->m_SpawnQueue.Num()) {
		BotFactory->SpawnNextBot();
	}
}

