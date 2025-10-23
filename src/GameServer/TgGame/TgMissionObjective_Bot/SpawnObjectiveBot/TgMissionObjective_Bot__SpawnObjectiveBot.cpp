#include "src/GameServer/TgGame/TgMissionObjective_Bot/SpawnObjectiveBot/TgMissionObjective_Bot__SpawnObjectiveBot.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgMissionObjective_Bot__SpawnObjectiveBot::Call(ATgMissionObjective_Bot *ObjectiveBot, void *edx) {

	Logger::Log("debug", "TgMissionObjective_Bot__SpawnObjectiveBot::Call\n");

	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	UWorld* World = (UWorld*)Globals::Get().GWorld;
	AWorldInfo* WorldInfo = (AWorldInfo*)World__GetWorldInfo::CallOriginal(World, nullptr, 0);

	if (ObjectiveBot->m_nMapObjectId == 11324) {
		ObjectiveBot->s_nBotId = 1400;
	}
	Logger::Log("debug", "s_nBotId = %d", ObjectiveBot->s_nBotId);

	ATgPawn* Bot = (ATgPawn*)Game->SpawnBotById(ObjectiveBot->s_nBotId, ObjectiveBot->Location, ObjectiveBot->Rotation, 0, nullptr, 1, nullptr, 0, nullptr, 0);

	if (Bot == nullptr) {
		Logger::Log("debug", "Bot == nullptr\n");
		return;
	} else {
		Logger::Log("debug", "Bot != nullptr\n");
	}

	Bot->bAlwaysRelevant = 1;

	// if (!Globals::Get().GWorldInfo) {
	// 	bool bFirstSkipped = false;
	// 	for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
	// 		if (UObject::GObjObjects()->Data[i]) {
	// 			if (strcmp(UObject::GObjObjects()->Data[i]->GetFullName(), "WorldInfo TheWorld.PersistentLevel.WorldInfo") == 0) {
	// 				if (!bFirstSkipped) {
	// 					bFirstSkipped = true;
	// 					continue;
	// 				}
	//
	// 				Globals::Get().GWorldInfo = reinterpret_cast<AWorldInfo*>(UObject::GObjObjects()->Data[i]);
	// 				break;
	// 			}
	// 		}
	// 	}
	// }

	// AWorldInfo* WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;

	// Bot->Owner = ObjectiveBot;

	// ObjectiveBot->Owner = WorldInfo;
	// ObjectiveBot->Base = WorldInfo;
	ObjectiveBot->Role = 3;
	ObjectiveBot->RemoteRole = 1;
	// ObjectiveBot->Owner = WorldInfo;
	// ObjectiveBot->Base = WorldInfo;

	ObjectiveBot->r_ObjectiveBot = Bot;
	ObjectiveBot->r_ObjectiveBotInfo = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	ObjectiveBot->r_bIsActive = 1;
	ObjectiveBot->r_eStatus = 3;

	

	ObjectiveBot->bNetDirty = 1;
	ObjectiveBot->bNetInitial = 1;
	ObjectiveBot->bForceNetUpdate = 1;
	ObjectiveBot->bSkipActorPropertyReplication = 0;

	// Bot->Owner = WorldInfo;
	// Bot->Base = WorldInfo;

	// Bot->Base = WorldInfo;
	// Bot->Owner = WorldInfo;
	ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
	if (ObjectiveBot->nDefaultOwnerTaskForce == 1) {
		BotRep->r_TaskForce = GTeamsData.Attackers;
		BotRep->Team = GTeamsData.Attackers;
		BotRep->SetTeam(GTeamsData.Attackers);
		Bot->NotifyTeamChanged();
	} else {
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

	BotRep->bNetDirty = 1;
	BotRep->bNetInitial = 1;
	BotRep->bForceNetUpdate = 1;
	BotRep->bSkipActorPropertyReplication = 0;

	if (ObjectiveBot->Base != nullptr) {
		Logger::Log("debug", "ObjectiveBot->Base %s\n", ObjectiveBot->Base->GetFullName());
	} else {
		Logger::Log("debug", "ObjectiveBot->Base == nullptr\n");
	}

	if (ObjectiveBot->Owner != nullptr) {
		Logger::Log("debug", "ObjectiveBot->Owner %s\n", ObjectiveBot->Owner->GetFullName());
	} else {
		Logger::Log("debug", "ObjectiveBot->Owner == nullptr\n");
	}

	if (Bot->Base != nullptr) {
		Logger::Log("debug", "Bot->Base %s\n", Bot->Base->GetFullName());
	} else {
		Logger::Log("debug", "Bot->Base == nullptr\n");
	}

	if (Bot->Owner != nullptr) {
		Logger::Log("debug", "Bot->Owner %s\n", Bot->Owner->GetFullName());
	} else {
		Logger::Log("debug", "Bot->Owner == nullptr\n");
	}

	Logger::Log("debug", "TgMissionObjective_Bot__SpawnObjectiveBot END\n");
	// s_nBotId
}

