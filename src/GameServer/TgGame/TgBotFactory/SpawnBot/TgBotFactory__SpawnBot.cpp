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
	static int nCycle = 0;

	Logger::Log("debug", "TgBotFactory::SpawnBot START\n");
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	AWorldInfo* WorldInfo = (AWorldInfo*)Globals::Get().GWorldInfo;


	UObject__CollectGarbage::bDisableGarbageCollection = true;

	ATgAIController* PlayerController = (ATgAIController*)Game->Spawn(ClassPreloader::GetTgAIControllerClass(), WorldInfo /*BotFactory->Owner*/, FName(), BotFactory->Location, BotFactory->Rotation, nullptr, 1);


	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), PlayerController->PlayerReplicationInfo, FName(), BotFactory->Location, BotFactory->Rotation, nullptr, 1);

	if (nCycle == 0) {
		newpawn->r_nPhysicalType = GA_G::PHYSICAL_TYPE_VALUE_ID_GUARDIAN;
		newpawn->r_nBodyMeshAsmId = GA_G::BODY_ASM_ID_SUPPORT_GUARDIAN;//0x5cc;
		newpawn->s_nCharacterId = GA_G::BOT_TYPE_VALUE_ID_GUARDIAN;//373;
		// newpawn->r_CustomCharacterAssembly.SuitMeshId = GA_G::BODY_ASM_ID_SUPPORT_GUARDIAN;
	} else if (nCycle == 1) {
		newpawn->r_nPhysicalType = GA_G::PHYSICAL_TYPE_VALUE_ID_AGENT;
		newpawn->r_nBodyMeshAsmId = GA_G::BODY_ASM_ID_ELITE_HELOT;
		newpawn->s_nCharacterId = GA_G::BOT_TYPE_VALUE_ID_ELITE;
		// newpawn->r_CustomCharacterAssembly.SuitMeshId = GA_G::BODY_ASM_ID_ELITE_HELOT;
	} else if (nCycle == 2) {
		newpawn->r_nPhysicalType = GA_G::PHYSICAL_TYPE_VALUE_ID_AGENT;
		newpawn->r_nBodyMeshAsmId = GA_G::BODY_ASM_ID_ELITE_TECHRO;
		newpawn->s_nCharacterId = GA_G::BOT_TYPE_VALUE_ID_ELITE;
		// newpawn->r_CustomCharacterAssembly.SuitMeshId = GA_G::BODY_ASM_ID_ELITE_TECHRO;
	} else if (nCycle == 3) {
		newpawn->r_nPhysicalType = GA_G::PHYSICAL_TYPE_VALUE_ID_AGENT;
		newpawn->r_nBodyMeshAsmId = GA_G::BODY_ASM_ID_ELITE_ASSASSIN;
		newpawn->s_nCharacterId = GA_G::BOT_TYPE_VALUE_ID_AGENT;
		// newpawn->r_CustomCharacterAssembly.SuitMeshId = GA_G::BODY_ASM_ID_ELITE_ASSASSIN;
	}

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

	// newpawn->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
	// newpawn->r_CustomCharacterAssembly.HairMeshId = -1;
	// newpawn->r_CustomCharacterAssembly.HelmetMeshId = -1;
	// newpawn->r_CustomCharacterAssembly.SkinToneParameterId = 0;
	// newpawn->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
	// newpawn->r_CustomCharacterAssembly.EyeColorParameterId = 0;
	// newpawn->r_CustomCharacterAssembly.bBald = true;
	// newpawn->r_CustomCharacterAssembly.bHideHelmet = true;
	// newpawn->r_CustomCharacterAssembly.bValidCustomAssembly = true;
	// newpawn->r_CustomCharacterAssembly.bHalfHelmet = false;
	// newpawn->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
	// newpawn->r_CustomCharacterAssembly.HeadFlairId = -1;
	// newpawn->r_CustomCharacterAssembly.SuitFlairId = -1;
	// newpawn->r_CustomCharacterAssembly.JetpackTrailId = 7638;
	// newpawn->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newpawn->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newpawn->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newpawn->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
	// newpawn->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;

	// newpawn->r_nSkillGroupSetId = 0;//GA_G::SKILL_GROUP_SET_ID_MEDIC;

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
	// newpawn->r_EffectManager->bForceNetUpdate = 1;
	// newpawn->r_EffectManager->bAlwaysRelevant = 1;

	// PlayerController->Role = 3;
	// PlayerController->RemoteRole = 1;
	// PlayerController->bNetInitial = 1;
	// PlayerController->bNetDirty = 1;
	// PlayerController->bForceNetUpdate = 1;
	// PlayerController->bReplicateMovement = 1;

	newpawn->bNetInitial = 1;
	newpawn->bNetDirty = 1;
	// newpawn->bForceNetUpdate = 1;
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

	newrepplayer->eventSetPlayerName(FString(L"Elite Helot"));
	// newrepplayer->SetTeam(GTeamsData.Attackers);
	// newrepplayer->r_TaskForce = GTeamsData.Attackers;
	// if (bAttacker) {
	// 	newrepplayer->r_TaskForce = GTeamsData.Attackers;
	// 	newrepplayer->SetTeam(GTeamsData.Attackers);
	// 	bAttacker = false;
	// } else {
		newrepplayer->r_TaskForce = GTeamsData.Defenders;
		newrepplayer->SetTeam(GTeamsData.Defenders);
	// 	bAttacker = true;
	// }
	// newrepplayer->SetPlayerTeam(GTeamsData.Attackers);
	// newrepplayer->Team = GTeamsData.Attackers;
	newrepplayer->bNetDirty = 1;
	// newrepplayer->bForceNetUpdate = 1;
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

	if (nCycle == 0) {
		void* BotConfig = (void*)CAmBot__LoadBotMarshal::m_BotPointers[GA_G::BOT_ID_ELITE_HELOT];
		TgAIController__InitBehavior::CallOriginal(PlayerController, edx, BotConfig, BotFactory);
	} else if (nCycle == 1) {
		void* BotConfig = (void*)CAmBot__LoadBotMarshal::m_BotPointers[GA_G::BOT_ID_SUPPORT_GUARDIAN];
		TgAIController__InitBehavior::CallOriginal(PlayerController, edx, BotConfig, BotFactory);
	} else if (nCycle == 2) {
		void* BotConfig = (void*)CAmBot__LoadBotMarshal::m_BotPointers[GA_G::BOT_ID_ELITE_TECHRO];
		TgAIController__InitBehavior::CallOriginal(PlayerController, edx, BotConfig, BotFactory);
	} else if (nCycle == 3) {
		void* BotConfig = (void*)CAmBot__LoadBotMarshal::m_BotPointers[GA_G::BOT_ID_ELITE_ASSASSIN];
		TgAIController__InitBehavior::CallOriginal(PlayerController, edx, BotConfig, BotFactory);
	}
	// void* BotConfig = (void*)CAmBot__LoadBotMarshal::m_BotPointers[GA_G::BOT_ID_SUPPORT_GUARDIAN];
	// TgAIController__InitBehavior::CallOriginal(PlayerController, edx, BotConfig, BotFactory);

	newpawn->NetUpdateFrequency = 15.0f;
	newpawn->NetPriority = 1.2f;

	newpawn->r_EffectManager->NetUpdateFrequency = 1.0f;
	newpawn->r_EffectManager->NetPriority = 1.0f;

	newpawn->PlayerReplicationInfo->NetUpdateFrequency = 1.0f;
	newpawn->PlayerReplicationInfo->NetPriority = 1.0f;


	nCycle++;
	if (nCycle > 1) {
		nCycle = 0;
	}
}

