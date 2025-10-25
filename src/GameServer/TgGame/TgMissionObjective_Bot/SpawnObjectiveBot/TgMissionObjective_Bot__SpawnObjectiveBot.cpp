#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgMissionObjective_Bot__SpawnObjectiveBot::Call(ATgMissionObjective_Bot *ObjectiveBot, void *edx) {

	Logger::Log("debug", "TgMissionObjective_Bot__SpawnObjectiveBot::Call\n");

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	UWorld* World = (UWorld*)Globals::Get().GWorld;
	AWorldInfo* WorldInfo = (AWorldInfo*)World__GetWorldInfo::CallOriginal(World, nullptr, 0);

	if (TgBotFactory__LoadObjectConfig::m_missionObjectiveBotToBotFactoryId[ObjectiveBot->m_nMapObjectId]) {
		int nBotFactoryId = TgBotFactory__LoadObjectConfig::m_missionObjectiveBotToBotFactoryId[ObjectiveBot->m_nMapObjectId];
		while (TgBotFactory__LoadObjectConfig::m_loadedBotFactories.find(nBotFactoryId) == TgBotFactory__LoadObjectConfig::m_loadedBotFactories.end()) {
			Sleep(100);
		}
		if (TgBotFactory__LoadObjectConfig::m_loadedBotFactories[nBotFactoryId]) {
			ATgBotFactory* BotFactory = TgBotFactory__LoadObjectConfig::m_loadedBotFactories[nBotFactoryId];

			for (int i = 0; i < BotFactory->LocationList.Num(); i++) {
				BotFactory->LocationList.Data[i]->Location = ObjectiveBot->Location;
			}

			BotFactory->SpawnNextBot();
			ATgPawn* Bot = TgBotFactory__SpawnNextBot::m_lastSpawnedBot[nBotFactoryId];
			// Bot->Location = ObjectiveBot->Location;

			// ATgAIController* AIController = (ATgAIController*)Bot->Controller;
			// AIController->m_PatrolPath.Clear();
			// AIController->m_PatrolPath.Add(BotFactory->LocationList.Data[0]);

			if (Bot == nullptr) {
				Logger::Log("debug", "Bot == nullptr\n");
				return;
			} else {
				Logger::Log("debug", "Bot != nullptr\n");
			}

			Bot->bAlwaysRelevant = 1;
			ObjectiveBot->Role = 3;
			ObjectiveBot->RemoteRole = 1;
			ObjectiveBot->r_ObjectiveBot = Bot;
			ObjectiveBot->r_ObjectiveBotInfo = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
			ObjectiveBot->r_bIsActive = 1;
			ObjectiveBot->r_eStatus = 3;
			ObjectiveBot->bNetDirty = 1;
			ObjectiveBot->bNetInitial = 1;
			ObjectiveBot->bForceNetUpdate = 1;
			ObjectiveBot->bSkipActorPropertyReplication = 0;

			Bot->Role = 3;
			Bot->RemoteRole = 1;
			Bot->bNetDirty = 1;
			Bot->bNetInitial = 1;
			Bot->bForceNetUpdate = 1;
			Bot->bSkipActorPropertyReplication = 0;

			ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
			BotRep->bNetDirty = 1;
			BotRep->bNetInitial = 1;
			BotRep->bForceNetUpdate = 1;
			BotRep->bSkipActorPropertyReplication = 0;
		}
	}

	Logger::Log("debug", "TgMissionObjective_Bot__SpawnObjectiveBot END\n");
}

