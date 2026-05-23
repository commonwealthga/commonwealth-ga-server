#include "src/GameServer/TgGame/TgBotFactory/SpawnNextBot/TgBotFactory__SpawnNextBot.hpp"
#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, ATgPawn*> TgBotFactory__SpawnNextBot::m_lastSpawnedBot;
static int nMaxInventoryId = 5000;  // start well above any player inventory IDs

void __fastcall TgBotFactory__SpawnNextBot::Call(ATgBotFactory *BotFactory, void *edx) {

	if (Logger::IsChannelEnabled("tgbotfactory")) {
		Logger::Log("tgbotfactory", "[%s] %s SpawnNextBot mapObjectId=%d\n", Logger::GetTime(), BotFactory->GetName(), BotFactory->m_nMapObjectId);
	}

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

			// Defensive: some factories (e.g. SDColony03's Kismet-driven
			// spawners) report Num()>0 but have a null Data pointer or null
			// individual entries — original game populated LocationList at
			// runtime via Kismet, we don't. Without these guards we crash
			// at ANavigationPoint->Location (offset 0x54..0x5C from null).
			if (BotFactory->LocationList.Data == nullptr ||
			    BotFactory->s_nCurLocationIndex >= BotFactory->LocationList.Num()) {
				Logger::Log("tgbotfactory",
					"  SpawnNextBot: LocationList Data=null or idx OOB for factory %d "
					"(num=%d idx=%d) — skipping spawn\n",
					BotFactory->m_nMapObjectId,
					BotFactory->LocationList.Num(),
					BotFactory->s_nCurLocationIndex);
				continue;
			}
			ANavigationPoint* BotStart = BotFactory->LocationList.Data[BotFactory->s_nCurLocationIndex];
			if (BotStart == nullptr) {
				Logger::Log("tgbotfactory",
					"  SpawnNextBot: LocationList[%d] is null for factory %d — skipping spawn\n",
					BotFactory->s_nCurLocationIndex, BotFactory->m_nMapObjectId);
				continue;
			}

			Logger::Log("tgbotfactory", " Spawning bot %d %s at X=%d,Y=%d,Z=%d",
				randomSpawnEntry.EnemyBotId, randomSpawnEntry.ReferenceName.c_str(),
				BotStart->Location.X, BotStart->Location.Y, BotStart->Location.Z);

			FVector SpawnLocation;
			SpawnLocation.X = BotStart->Location.X;
			SpawnLocation.Y = BotStart->Location.Y;
			SpawnLocation.Z = BotStart->Location.Z;

			// Lift Z by halfHeight + 5uu floor buffer. Same pattern SpawnPet and
			// SpawnDeployable use — APawn.Location is at the cylinder CENTER,
			// and NavigationPoints (BotStart) are placed at ground level by
			// the level designer. Without this lift, the bot's cylinder center
			// sits at ground; cylinder extends halfHeight below ground. For
			// meshes whose model pivot is at the CENTER (not the feet — common
			// for big bosses, hovers, drones), the lower half of the mesh
			// renders below the ground plane → "sunken into terrain" look.
			// Bots with crouchable leg rigs (Shrike) mask this by bending
			// knees, which reads as the bot being permanently crouched.
			//
			// halfHeight comes from asm_data_set_assembly_meshes.collision_height
			// via the bot's body_asm_id — same source SetCollisionSize uses, so
			// the lift matches the cylinder size we apply in SpawnBotById.
			{
				float radius = 0.f, halfHeight = 0.f;
				TgGame__SpawnBotById::GetBotCollisionCylinder(randomSpawnEntry.EnemyBotId, &radius, &halfHeight);
				if (halfHeight > 0.0f) {
					SpawnLocation.Z += halfHeight + 5.0f;
				}
			}


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

			if (Bot == nullptr || Bot->Controller == nullptr) {
				Logger::Log("tgbotfactory",
					"  SpawnNextBot: SpawnBotById returned null pawn/controller "
					"for bot_id=%d (factory %d) — skipping post-spawn wiring\n",
					randomSpawnEntry.EnemyBotId, BotFactory->m_nMapObjectId);
				continue;
			}
			ATgAIController* AIController = (ATgAIController*)Bot->Controller;
			// IMPORTANT: do NOT clear AIController->bIsPlayer. Earlier we did
			// this to filter ClientAddKilled targets, but bIsPlayer is also
			// used by turret target acquisition + AI threat lists as a
			// "valid combatant" gate — clearing it broke turrets targeting
			// bots. Use class-name strstr "PlayerController" for the
			// real-player check instead.
			for (int j = 0; j < BotFactory->PatrolPath.Num(); j++) {
				AIController->m_PatrolPath.Add(BotFactory->PatrolPath.Data[j]);
			}
			// Single-point fallback so the bot can stay in `state Patrol`
			// rather than slipping into `state Wander` (TgAIController.uc
			// line 3903 — PickDestination picks arbitrary nav points and
			// walks into walls). GetNextPatrolPoint returns m_PatrolPath[0]
			// when Length is small, so feeding it BotStart turns the patrol
			// loop into "stand at spawn point". Same effect as the original
			// Kismet flow on factories without a designer-placed patrol
			// route. Bots whose factory DOES have a patrol route are
			// unaffected — the fallback only fires when the route is empty.
			if (AIController->m_PatrolPath.Num() == 0 && BotStart != nullptr) {
				AIController->m_PatrolPath.Add(BotStart);
			}

			ATgRepInfo_Player* BotRep = (ATgRepInfo_Player*)Bot->PlayerReplicationInfo;
			if (BotRep == nullptr) {
				Logger::Log("tgbotfactory",
					"  SpawnNextBot: bot_id=%d (factory %d) has no PlayerReplicationInfo — "
					"skipping team / replication wiring\n",
					randomSpawnEntry.EnemyBotId, BotFactory->m_nMapObjectId);
				continue;
			}

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


			// Device equipping is handled by SpawnBotById (which was called above).
			// Calling CreateEquipDevice here with a different invId would destroy the devices
			// SpawnBotById just set up (CreateEquipDevice destroys the existing device when invId mismatches).

			// Bot->Role = 3;
			// Bot->RemoteRole = 1;
			// // Bot->bNetDirty = 1;
			// Bot->bNetInitial = 1;
			// Bot->bForceNetUpdate = 1;
			// Bot->bSkipActorPropertyReplication = 0;
			// // Bot->bAlwaysRelevant = 1;
			//
			// BotRep->Role = 3;
			// BotRep->RemoteRole = 1;
			// // BotRep->bNetDirty = 1;
			// BotRep->bNetInitial = 1;
			// BotRep->bForceNetUpdate = 1;
			// BotRep->bSkipActorPropertyReplication = 0;
			// // BotRep->bAlwaysRelevant = 1;

		}
	}

	if (BotFactory->s_nCurListIndex + 1 < BotFactory->m_SpawnQueue.Num()) {
		BotFactory->SpawnNextBot();
	}
}

