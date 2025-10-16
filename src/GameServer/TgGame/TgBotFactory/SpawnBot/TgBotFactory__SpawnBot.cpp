#include "src/GameServer/TgGame/TgBotFactory/SpawnBot/TgBotFactory__SpawnBot.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/TgGame/TgAIController/InitBehavior/TgAIController__InitBehavior.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotBehaviorMarshal/CAmBot__LoadBotBehaviorMarshal.hpp"
#include "src/GameServer/Core/UObject/CollectGarbage/UObject__CollectGarbage.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Macros.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgBotFactory__SpawnBot::Call(ATgBotFactory* BotFactory, void* edx) {
	Logger::Log("debug", "TgBotFactory::SpawnBot START\n");
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	AWorldInfo* WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;


	UObject__CollectGarbage::bDisableGarbageCollection = true;

	ATgAIController* PlayerController = (ATgAIController*)Game->Spawn(ClassPreloader::GetTgAIControllerClass(), WorldInfo /*BotFactory->Owner*/, FName(), BotFactory->Location, BotFactory->Rotation, nullptr, 1);


	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), PlayerController->PlayerReplicationInfo, FName(), BotFactory->Location, BotFactory->Rotation, nullptr, 1);

	newpawn->r_nPhysicalType = GA_G::PHYSICAL_TYPE_VALUE_ID_AGENT;
	newpawn->ReplicatedCollisionType = newpawn->CollisionType;
	newpawn->r_nHealthMaximum = 4500;
	
	// newpawn->r_nProfileId = 567; // medic
	// newpawn->r_bDisableAllDevices = 0;
	// newpawn->r_bEnableEquip = 1;
	// newpawn->r_bEnableSkills = 1;
	// newpawn->r_bEnableCrafting = 1;
	// newpawn->r_bIsStealthed = 0;
	newpawn->r_bIsBot = 1;
	newpawn->r_fCurrentPowerPool = 100;
	newpawn->r_fMaxPowerPool = 100;
	newpawn->r_nXp = 999999;
	newpawn->Health = 4500;
	newpawn->HealthMax = 4500;

	newpawn->r_nBodyMeshAsmId = GA_G::BODY_ASM_ID_ELITE_TECHRO;//0x5cc;
	newpawn->r_CustomCharacterAssembly.SuitMeshId = 1225;
	newpawn->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
	newpawn->r_CustomCharacterAssembly.HairMeshId = 1974;
	newpawn->r_CustomCharacterAssembly.HelmetMeshId = -1;
	newpawn->r_CustomCharacterAssembly.SkinToneParameterId = 0;
	newpawn->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
	newpawn->r_CustomCharacterAssembly.EyeColorParameterId = 0;
	newpawn->r_CustomCharacterAssembly.bBald = false;
	newpawn->r_CustomCharacterAssembly.bHideHelmet = false;
	newpawn->r_CustomCharacterAssembly.bValidCustomAssembly = true;
	newpawn->r_CustomCharacterAssembly.bHalfHelmet = false;
	newpawn->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
	newpawn->r_CustomCharacterAssembly.HeadFlairId = -1;
	newpawn->r_CustomCharacterAssembly.SuitFlairId = -1;
	newpawn->r_CustomCharacterAssembly.JetpackTrailId = 7638;
	newpawn->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newpawn->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newpawn->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newpawn->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newpawn->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newpawn->r_nSkillGroupSetId = 0;//GA_G::SKILL_GROUP_SET_ID_MEDIC;
	newpawn->s_nCharacterId = GA_G::BOT_TYPE_VALUE_ID_ELITE_TECHRO;//373;

	PlayerController->Pawn = newpawn;
	newpawn->Controller = PlayerController;
	newpawn->Owner = PlayerController->PlayerReplicationInfo;
	newpawn->PlayerReplicationInfo = PlayerController->PlayerReplicationInfo;

	newpawn->r_EffectManager->Owner = newpawn;
	newpawn->r_EffectManager->r_Owner = newpawn;
	newpawn->r_EffectManager->Base = newpawn;
	newpawn->r_EffectManager->Role = 3;
	newpawn->r_EffectManager->RemoteRole = 1;
	newpawn->r_EffectManager->bNetInitial = 1;
	newpawn->r_EffectManager->bNetDirty = 1;
	newpawn->r_EffectManager->bForceNetUpdate = 1;
	// newpawn->r_EffectManager->bAlwaysRelevant = 1;

	PlayerController->Role = 3;
	PlayerController->RemoteRole = 1;
	PlayerController->bNetInitial = 1;
	PlayerController->bNetDirty = 1;
	PlayerController->bForceNetUpdate = 1;
	PlayerController->bReplicateMovement = 1;

	newpawn->bNetInitial = 1;
	newpawn->bNetDirty = 1;
	newpawn->bForceNetUpdate = 1;
	newpawn->bReplicateMovement = 1;


	ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(PlayerController->PlayerReplicationInfo);
	// newrepplayer->Team = defenders;
	newrepplayer->bBot = 1;
	// newrepplayer->r_CustomCharacterAssembly.SuitMeshId = 1225;
	// newrepplayer->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
	// newrepplayer->r_CustomCharacterAssembly.HairMeshId = 1974;
	// newrepplayer->r_CustomCharacterAssembly.HelmetMeshId = -1;
	// newrepplayer->r_CustomCharacterAssembly.SkinToneParameterId = 0;
	// newrepplayer->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
	// newrepplayer->r_CustomCharacterAssembly.EyeColorParameterId = 0;
	// newrepplayer->r_CustomCharacterAssembly.bBald = false;
	// newrepplayer->r_CustomCharacterAssembly.bHideHelmet = false;
	// newrepplayer->r_CustomCharacterAssembly.bValidCustomAssembly = true;
	// newrepplayer->r_CustomCharacterAssembly.bHalfHelmet = false;
	// newrepplayer->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
	// newrepplayer->r_CustomCharacterAssembly.HeadFlairId = -1;
	// newrepplayer->r_CustomCharacterAssembly.SuitFlairId = -1;
	// newrepplayer->r_CustomCharacterAssembly.JetpackTrailId = 7638;
	// newrepplayer->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newrepplayer->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newrepplayer->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newrepplayer->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newrepplayer->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newrepplayer->r_nProfileId = 567; // medic
	newrepplayer->r_nHealthCurrent = 4500;
	newrepplayer->r_nHealthMaximum = 4500;
	newrepplayer->r_nCharacterId = newpawn->s_nCharacterId;
	newrepplayer->r_nLevel = 50;
	// newrepplayer->r_sOrigPlayerName = FString(L"Zaxik");
	newrepplayer->r_PawnOwner = newpawn;
	newrepplayer->r_ApproxLocation = newpawn->Location;
	// newrepplayer->PlayerName = FString(L"Zaxik");

	newrepplayer->eventSetPlayerName(FString(L"ELite Techro"));
	// newrepplayer->SetTeam(GTeamsData.Attackers);
	// newrepplayer->r_TaskForce = GTeamsData.Attackers;
	newrepplayer->r_TaskForce = GTeamsData.Defenders;
	newrepplayer->SetTeam(GTeamsData.Defenders);
	// newrepplayer->SetPlayerTeam(GTeamsData.Attackers);
	// newrepplayer->Team = GTeamsData.Attackers;
	newrepplayer->bNetDirty = 1;
	newrepplayer->bForceNetUpdate = 1;
	newrepplayer->bSkipActorPropertyReplication = 0;
	newrepplayer->bOnlyDirtyReplication = 0;
	newrepplayer->bNetInitial = 1;

	PlayerController->Possess(newpawn, 0, 1);
	newpawn->ApplyPawnSetup();
	newpawn->WaitForInventoryThenDoPostPawnSetup();
	// newpawn->eventPostPawnSetup();

	if (newpawn->r_EffectManager == nullptr) {
		Logger::Log("debug", " bot effect manager is null\n");
	} else {
		Logger::Log("debug", " bot effect manager is not null\n  ->bOnlyRelevantToOwner: %d", newpawn->r_EffectManager->bOnlyRelevantToOwner);

	}

	TgAIController__InitBehavior::CallOriginal(PlayerController, edx, (void*)CAmBot__LoadBotMarshal::m_BotPointers[1432], BotFactory);

	if (newpawn->Mesh == nullptr) {
		Logger::Log("debug", " bot mesh is null\n");
	} else {
		Logger::Log("debug", " bot mesh is not null\n");
	}

	Sleep(100);
	newpawn->r_EffectManager->bAlwaysRelevant = 0;
	return;




	ATgAIController* botai = (ATgAIController*)Game->Spawn(ClassPreloader::GetTgAIControllerClass(), Game /*BotFactory->Owner*/, FName(), BotFactory->Location, BotFactory->Rotation, nullptr, 1);


	// if (botai) {
	// 	ATgPawn_Character* bot = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), botai, FName(), BotFactory->Location, BotFactory->Rotation, nullptr, 1);
	// 	botai->m_pBot.Dummy = CAmBot__LoadBotMarshal::m_BotPointers[1432];
	// 	botai->Pawn = bot;
	// 	bot->Controller = botai;
	// 	bot->Owner = botai;
	// 	bot->Role = 3;
	// 	bot->RemoteRole = 1;
	// 	Game->RestartPlayer(botai);
	// 	if (botai->Pawn) {
	// 		botai->Pawn->Location = BotFactory->Location;
	// 	}
	// }
	// return;




	if (botai) {

		Logger::Log("debug", " AI controller spawned\n");
		botai->Owner = Game;

		ATgPawn_Character* bot = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), botai, FName(), BotFactory->Location, BotFactory->Rotation, nullptr, 1);

		if (bot) {

			// bot->Role = 3;
			// bot->RemoteRole = 1;
			
			//CAmBot__LoadBotBehaviorMarshal::m_BotBehaviorPointers[730]
			TgAIController__InitBehavior::CallOriginal(botai, edx, (void*)CAmBot__LoadBotMarshal::m_BotPointers[1432], BotFactory);
			// botai->m_pBaseBehavior.Dummy = CAmBot__LoadBotBehaviorMarshal::m_BotBehaviorPointers[730];
			// botai->m_pBot.Dummy = CAmBot__LoadBotMarshal::m_BotPointers[1432];

			Logger::Log("debug", " bot spawned\n");
			// botai->m_pBot.Dummy = (int)bot;


			botai->Pawn = bot;
			bot->Controller = botai;
			bot->Owner = botai;

			// botai->bNetInitial = 1;
			// botai->bNetDirty = 1;
			// botai->bForceNetUpdate = 1;
			// botai->bReplicateMovement = 1;

			bot->PlayerReplicationInfo = botai->PlayerReplicationInfo;
			bot->r_nHeadMeshAsmId = 1605;
			bot->r_nBodyMeshAsmId = 1894;

			// bot->r_nBodyMeshAsmId = 1225;
			// bot->r_CustomCharacterAssembly.SuitMeshId = 1225;
			// bot->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
			// bot->r_CustomCharacterAssembly.HairMeshId = 1974;
			// bot->r_CustomCharacterAssembly.HelmetMeshId = -1;
			// bot->r_CustomCharacterAssembly.SkinToneParameterId = 0;
			// bot->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
			// bot->r_CustomCharacterAssembly.EyeColorParameterId = 0;
			// bot->r_CustomCharacterAssembly.bBald = false;
			// bot->r_CustomCharacterAssembly.bHideHelmet = false;
			// bot->r_CustomCharacterAssembly.bValidCustomAssembly = true;
			// bot->r_CustomCharacterAssembly.bHalfHelmet = false;
			// bot->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
			// bot->r_CustomCharacterAssembly.HeadFlairId = -1;
			// bot->r_CustomCharacterAssembly.SuitFlairId = -1;
			// bot->r_CustomCharacterAssembly.JetpackTrailId = 7638;
			// bot->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
			// bot->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
			// bot->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
			// bot->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
			// bot->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;

			bot->r_bIsBot = 1;
			bot->Health = 4500;
			bot->HealthMax = 4500;
			bot->r_nReplicateDying = 0;

			bot->bNetInitial = 1;
			bot->bNetDirty = 1;
			bot->bForceNetUpdate = 1;
			bot->bReplicateMovement = 1;
			bot->bSkipActorPropertyReplication = 0;
			bot->bOnlyDirtyReplication = 0;

			bot->bHidden = 0;

			// bot->r_nResetCharacter = 1;

			// botai->eventSetWhatToDoNext(8, 185);

			Logger::Log("debug", "bot controller state: %s\n", botai->GetStateName().GetName());
			Logger::Log("debug", "bot pawn state: %s\n", bot->GetStateName().GetName());

			if (bot->Mesh == nullptr) {
				Logger::Log("debug", " bot mesh is null\n");
			} else {
				Logger::Log("debug", " bot mesh is not null\n");
			}

			ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(botai->PlayerReplicationInfo);
			// newrepplayer->Team = defenders;
			// newrepplayer->bAdmin = 1;

			// newrepplayer->r_CustomCharacterAssembly.SuitMeshId = 1225;
			// newrepplayer->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
			// newrepplayer->r_CustomCharacterAssembly.HairMeshId = 1974;
			// newrepplayer->r_CustomCharacterAssembly.HelmetMeshId = -1;
			// newrepplayer->r_CustomCharacterAssembly.SkinToneParameterId = 0;
			// newrepplayer->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
			// newrepplayer->r_CustomCharacterAssembly.EyeColorParameterId = 0;
			// newrepplayer->r_CustomCharacterAssembly.bBald = false;
			// newrepplayer->r_CustomCharacterAssembly.bHideHelmet = false;
			// newrepplayer->r_CustomCharacterAssembly.bValidCustomAssembly = true;
			// newrepplayer->r_CustomCharacterAssembly.bHalfHelmet = false;
			// newrepplayer->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
			// newrepplayer->r_CustomCharacterAssembly.HeadFlairId = -1;
			// newrepplayer->r_CustomCharacterAssembly.SuitFlairId = -1;
			// newrepplayer->r_CustomCharacterAssembly.JetpackTrailId = 7638;
			// newrepplayer->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
			// newrepplayer->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
			// newrepplayer->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
			// newrepplayer->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
			// newrepplayer->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;

			newrepplayer->r_nProfileId = 0x2A7; // robo
			newrepplayer->r_nHealthCurrent = 4500;
			newrepplayer->r_nHealthMaximum = 4500;
			// newrepplayer->r_nCharacterId = bot->s_nCharacterId;
			newrepplayer->r_nLevel = 50;
			// newrepplayer->r_sOrigPlayerName = FString(L"Elite Techro");
			newrepplayer->r_PawnOwner = bot;
			newrepplayer->r_ApproxLocation = bot->Location;
			// newrepplayer->PlayerName = FString(L"Elite Techro");

			newrepplayer->bBot = 1;
			// newrepplayer->eventSetPlayerName(FString(L"Elite Techro"));
			// newrepplayer->SetTeam(GTeamsData.Attackers);
			newrepplayer->r_TaskForce = GTeamsData.Defenders;
			// newrepplayer->SetPlayerTeam(GTeamsData.Attackers);
			// newrepplayer->Team = GTeamsData.Attackers;
			newrepplayer->r_PawnOwner = bot;
			newrepplayer->bNetDirty = 1;
			newrepplayer->bForceNetUpdate = 1;
			newrepplayer->bSkipActorPropertyReplication = 0;
			newrepplayer->bOnlyDirtyReplication = 0;
			newrepplayer->bNetInitial = 1;

			bot->ApplyPawnSetup();
			bot->WaitForInventoryThenDoPostPawnSetup();


			botai->Possess(bot, 0, 1);

			// ATgRepInfo_TaskForce* attackers = GTeamsData.Attackers;
			// ATgRepInfo_TaskForce* defenders = GTeamsData.Defenders;

			// FTGTEAM_ENTRY newplayerteamentry;
			// newplayerteamentry.fsName = FString(L"Zaxik");
			// newplayerteamentry.fsMapName = FString(L"Tetra Pier");
			// newplayerteamentry.nHealth = 1300;
			// newplayerteamentry.nMaxHealth = 1300;
			// newplayerteamentry.bLeader = 0;
			// newplayerteamentry.pPrep = newrepplayer;
			//
			// TARRAY_INIT(attackers, TeamPlayersAttackers, FTGTEAM_ENTRY, 0x214, 32);
			// TARRAY_INIT(defenders, TeamPlayersDefenders, FTGTEAM_ENTRY, 0x214, 32);
			//
			// TARRAY_ADD(TeamPlayersAttackers, newplayerteamentry);

			Logger::Log("debug", " bot configured\n");

		} else {
			Logger::Log("debug", " bot failed to spawn\n");
		}

	} else {
		Logger::Log("debug", " AI controller failed to spawn\n");
	}
	Logger::Log("debug", "TgBotFactory::SpawnBot END\n");
}

