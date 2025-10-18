#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

ATgPawn_Character* __fastcall TgGame__SpawnPlayerCharacter::Call(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation) {

	Logger::Log("debug", "MINE TgGame__SpawnPlayerCharacter START\n");

	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), PlayerController, FName(), SpawnLocation, PlayerController->Rotation, nullptr, 1);
	ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(PlayerController->PlayerReplicationInfo);

	// Logger::DumpMemory("newpawn", newpawn, 0x800, 0);


	// try equip jetpack
	ATgDevice* Jetpack = newpawn->CreateEquipDevice(0, GA_G::DEVICE_ID_MEDIC_CJP, GA_G::EQUIP_POINT_JETPACK_ID);
	if (Jetpack != nullptr) {
		Jetpack->m_bIsOffHand = 1;
		Jetpack->m_nDeviceType = 806;
		Jetpack->bNetInitial = 1;
		Jetpack->bNetDirty = 1;
		Jetpack->bForceNetUpdate = 1;
		Logger::Log("debug", "Jetpack = 0x%p, %s\n", Jetpack, Jetpack->GetFullName());
		Logger::Log("debug", "Jetpack->r_nInventoryId = %d", Jetpack->r_nInventoryId);

		if (Jetpack->s_InventoryObject != nullptr) {
			Logger::Log("debug", "Jetpack->s_InventoryObject = 0x%p, %s\n", Jetpack->s_InventoryObject, Jetpack->s_InventoryObject->GetFullName());
		} else {
			Logger::Log("debug", "Jetpack->s_InventoryObject = nullptr\n");
		}


		for (int i = 0; i < 25; i++) {
			newpawn->m_EquippedDevices[i] = Jetpack;
			newpawn->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
			newpawn->r_EquipDeviceInfo[i].nDeviceInstanceId = 1;
			newpawn->r_EquipDeviceInfo[i].nQualityValueId = 1165;

			newrepplayer->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
			newrepplayer->r_EquipDeviceInfo[i].nDeviceInstanceId = 1;
			newrepplayer->r_EquipDeviceInfo[i].nQualityValueId = 1165;
		}

		Jetpack->r_eEquippedAt = GA_G::EQUIP_POINT_JETPACK_ID;

		Jetpack->r_nInventoryId = 1;
		Jetpack->s_InventoryObject->m_InventoryData.nInvId = 1;

		TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)newpawn->InvManager + 0x1f0), edx, 1, Jetpack->s_InventoryObject);
		ATgDevice* JetpackFixed = newpawn->CreateEquipDevice(1, GA_G::DEVICE_ID_MEDIC_CJP, GA_G::EQUIP_POINT_JETPACK_ID);
		if (JetpackFixed != nullptr) {
			Logger::Log("debug", "JetpackFixed = 0x%p, %s\n", JetpackFixed, JetpackFixed->GetFullName());
		} else {
			Logger::Log("debug", "JetpackFixed = nullptr\n");
		}

		// newpawn->ForceUpdateEquippedDevices();
		// newpawn->UpdateClientDevices();
	} else {
		Logger::Log("debug", "Jetpack = nullptr\n");
	}

	newpawn->r_nPhysicalType = 860;
	newpawn->ReplicatedCollisionType = newpawn->CollisionType;
	newpawn->r_nHealthMaximum = 1300;
	newpawn->r_nProfileId = 567; // medic
	newpawn->r_bDisableAllDevices = 0;
	newpawn->r_bEnableEquip = 1;
	newpawn->r_bEnableSkills = 1;
	newpawn->r_bEnableCrafting = 1;
	newpawn->r_bIsStealthed = 0;
	newpawn->r_bIsBot = 0;
	newpawn->r_fCurrentPowerPool = 100;
	newpawn->r_fMaxPowerPool = 100;
	newpawn->r_nXp = 999999;
	newpawn->Health = 1300;
	newpawn->HealthMax = 1300;

	newpawn->r_nBodyMeshAsmId = 1225;//0x5cc;
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
	newpawn->r_nSkillGroupSetId = GA_G::SKILL_GROUP_SET_ID_MEDIC;
	newpawn->s_nCharacterId = 373;

	PlayerController->Pawn = newpawn;
	newpawn->Controller = PlayerController;

	// PlayerController->Role = 3;
	// PlayerController->RemoteRole = 2;
	PlayerController->bNetInitial = 1;
	PlayerController->bNetDirty = 1;
	PlayerController->bForceNetUpdate = 1;
	PlayerController->bReplicateMovement = 1;

	newpawn->PlayerReplicationInfo = PlayerController->PlayerReplicationInfo;

	newpawn->bNetInitial = 1;
	newpawn->bNetDirty = 1;
	newpawn->bForceNetUpdate = 1;
	newpawn->bReplicateMovement = 1;


	// newrepplayer->Team = defenders;
	newrepplayer->bAdmin = 1;
	newrepplayer->r_CustomCharacterAssembly.SuitMeshId = 1225;
	newrepplayer->r_CustomCharacterAssembly.HeadMeshId = GA_G::HEAD_ASM_ID_TROLL;
	newrepplayer->r_CustomCharacterAssembly.HairMeshId = 1974;
	newrepplayer->r_CustomCharacterAssembly.HelmetMeshId = -1;
	newrepplayer->r_CustomCharacterAssembly.SkinToneParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.SkinRaceParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.EyeColorParameterId = 0;
	newrepplayer->r_CustomCharacterAssembly.bBald = false;
	newrepplayer->r_CustomCharacterAssembly.bHideHelmet = false;
	newrepplayer->r_CustomCharacterAssembly.bValidCustomAssembly = true;
	newrepplayer->r_CustomCharacterAssembly.bHalfHelmet = false;
	newrepplayer->r_CustomCharacterAssembly.nGenderTypeId = GA_G::GENDER_TYPE_ID_MALE;
	newrepplayer->r_CustomCharacterAssembly.HeadFlairId = -1;
	newrepplayer->r_CustomCharacterAssembly.SuitFlairId = -1;
	newrepplayer->r_CustomCharacterAssembly.JetpackTrailId = 7638;
	newrepplayer->r_CustomCharacterAssembly.DyeList[0] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[1] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[2] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[3] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_CustomCharacterAssembly.DyeList[4] = GA_G::DYE_ID_NONE_MORE_BLACK;
	newrepplayer->r_nProfileId = 567; // medic
	newrepplayer->r_nHealthCurrent = 1300;
	newrepplayer->r_nHealthMaximum = 1300;
	newrepplayer->r_nCharacterId = newpawn->s_nCharacterId;
	newrepplayer->r_nLevel = 50;
	// newrepplayer->r_sOrigPlayerName = FString(L"Zaxik");
	newrepplayer->r_PawnOwner = newpawn;
	newrepplayer->r_ApproxLocation = newpawn->Location;
	// newrepplayer->PlayerName = FString(L"Zaxik");

	newrepplayer->eventSetPlayerName(FString(L"Zaxik"));
	// newrepplayer->SetTeam(GTeamsData.Attackers);
	// newrepplayer->r_TaskForce = GTeamsData.Attackers;
	newrepplayer->SetTeam(GTeamsData.Attackers);
	// newrepplayer->SetPlayerTeam(GTeamsData.Attackers);
	// newrepplayer->Team = GTeamsData.Attackers;
	newrepplayer->bNetDirty = 1;
	newrepplayer->bForceNetUpdate = 1;
	newrepplayer->bSkipActorPropertyReplication = 0;
	newrepplayer->bOnlyDirtyReplication = 0;
	newrepplayer->bNetInitial = 1;

	ATgRepInfo_TaskForce* attackers = GTeamsData.Attackers;
	ATgRepInfo_TaskForce* defenders = GTeamsData.Defenders;

	FTGTEAM_ENTRY newplayerteamentry;
	newplayerteamentry.fsName = FString(L"Zaxik");
	newplayerteamentry.fsMapName = FString(L"Tetra Pier");
	newplayerteamentry.nHealth = 1300;
	newplayerteamentry.nMaxHealth = 1300;
	newplayerteamentry.bLeader = 0;
	newplayerteamentry.pPrep = newrepplayer;

	TARRAY_INIT(attackers, TeamPlayersAttackers, FTGTEAM_ENTRY, 0x214, 32);
	TARRAY_INIT(defenders, TeamPlayersDefenders, FTGTEAM_ENTRY, 0x214, 32);

	TARRAY_ADD(TeamPlayersAttackers, newplayerteamentry);


	Logger::Log("debug", "MINE TgGame__SpawnPlayerCharacter END\n");

	return newpawn;
}

