#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/Constants/EquipSlot.hpp"
#include "src/GameServer/Constants/DeviceIds.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/TgGame/TgPawn/SyncPawnHealth/SyncPawnHealth.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/CreateDevice/AssemblyDatManager__CreateDevice.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/CommandLineParser/CommandLineParser.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

bool TgGame__SpawnPlayerCharacter::bEnemyGearSpawned = true;  // now handled in SpawnBotById

ATgPawn_Character* __fastcall TgGame__SpawnPlayerCharacter::Call(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation) {

	LogCallBegin();

	int32_t ConnectionIndex = (int32_t)((UNetConnection*)PlayerController->Player);

	Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: connection=%d\n", ConnectionIndex);

	// Read the player's selected class from connection data.
	uint32_t profileId = GClientConnectionsData[ConnectionIndex].PlayerInfo.selected_profile_id;

	// Validate: fall back to Assault (680) for unknown or unset profile IDs.
	if (profileId != 567 && profileId != 679 && profileId != 680 && profileId != 681) {
		Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: invalid profile_id=%d for connection=%d, falling back to Assault\n", profileId, ConnectionIndex);
		profileId = 681;
		// profileId = 680;
	}

	const ClassConfig& classConfig = GetClassConfig(profileId);

	// nPendingBotId = profileId so InitializeDefaultProps loads the correct class stats from DB.
	// For player classes, bot_id == profile_id in asm_data_set_bots.
	TgPawn__InitializeDefaultProps::nPendingBotId = profileId;
	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), PlayerController, FName(), SpawnLocation, PlayerController->Rotation, nullptr, 1);
	// nPendingBotId cleared inside InitializeDefaultProps::Call

	newpawn->r_nPawnId = 998;
	ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(PlayerController->PlayerReplicationInfo);

	PlayerController->Pawn = newpawn;
	newpawn->Controller = PlayerController;

	PlayerController->eventPossess(PlayerController->Pawn, 0, 0);

	// NOTE: don't call ClientSetCinematicMode here — we're still inside
	// PostLogin → RestartPlayer → SpawnDefaultPawnFor, i.e. BEFORE
	// GenericPlayerInitialization → ClientSetHUD spawns the real TgHUD_Game
	// on the client. Firing the RPC now would target the stale TgLoginHUD.
	// It's invoked from MarshalChannel__NotifyControlMessage::HandlePlayerConnected
	// after eventPostLogin completes.






	// undefined4 __fastcall TgDevice__IsOffhandJetpack(int param_1)
	// {
	//   if (((*(uint *)(param_1 + 0x22c) & 0x8000) != 0) && (*(int *)(param_1 + 0x298) == 0x326)) {
	// 	return 1;
	//   }
	//   return 0;
	// }
	// if (device->m_bIsOffHand && device->m_nDeviceType == 806) {

	// // Logger::DumpMemory("newpawn", newpawn, 0x800, 0);
	// int AgonizerSlot = newpawn->GetEquipPointByType(217);
	// ATgDevice* Agonizer = newpawn->CreateEquipDevice(0, GA_G::DEVICE_ID_AGONIZER, AgonizerSlot);
	// Agonizer->bNetInitial = 1;
	// Agonizer->bNetDirty = 1;
	// Agonizer->bForceNetUpdate = 1;
	//
	//
	// // try equip jetpack
	// ATgDevice* Jetpack = newpawn->CreateEquipDevice(0, GA_G::DEVICE_ID_MEDIC_CJP, GA_G::EQUIP_POINT_JETPACK_ID);
	// if (Jetpack != nullptr) {
	// 	Jetpack->m_bIsOffHand = 1;
	// 	Jetpack->m_nDeviceType = 806;
	// 	Jetpack->bNetInitial = 1;
	// 	Jetpack->bNetDirty = 1;
	// 	Jetpack->bForceNetUpdate = 1;
	// 	Logger::Log("debug", "Jetpack = 0x%p, %s\n", Jetpack, Jetpack->GetFullName());
	// 	Logger::Log("debug", "Jetpack->r_nInventoryId = %d", Jetpack->r_nInventoryId);
	//
	// 	if (Jetpack->s_InventoryObject != nullptr) {
	// 		Logger::Log("debug", "Jetpack->s_InventoryObject = 0x%p, %s\n", Jetpack->s_InventoryObject, Jetpack->s_InventoryObject->GetFullName());
	// 	} else {
	// 		Logger::Log("debug", "Jetpack->s_InventoryObject = nullptr\n");
	// 	}
	//
	//
	// 	for (int i = 0; i < 25; i++) {
	// 		if (i == AgonizerSlot) {
	// 			newpawn->m_EquippedDevices[i] = Agonizer;
	// 			newpawn->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_AGONIZER;
	// 			newpawn->r_EquipDeviceInfo[i].nDeviceInstanceId = nMaxInventoryId + 1;
	// 			newpawn->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	//
	// 			newrepplayer->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_AGONIZER;
	// 			newrepplayer->r_EquipDeviceInfo[i].nDeviceInstanceId = nMaxInventoryId + 1;
	// 			newrepplayer->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	// 		} else {
	// 			newpawn->m_EquippedDevices[i] = Jetpack;
	// 			newpawn->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
	// 			newpawn->r_EquipDeviceInfo[i].nDeviceInstanceId = nMaxInventoryId + 2;
	// 			newpawn->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	//
	// 			newrepplayer->r_EquipDeviceInfo[i].nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
	// 			newrepplayer->r_EquipDeviceInfo[i].nDeviceInstanceId = nMaxInventoryId + 2;
	// 			newrepplayer->r_EquipDeviceInfo[i].nQualityValueId = 1165;
	// 		}
	// 	}
	//
	// 	Jetpack->r_eEquippedAt = GA_G::EQUIP_POINT_JETPACK_ID;
	// 	Agonizer->r_eEquippedAt = AgonizerSlot;
	//
	// 	// for (int i = 0; i < Jetpack->m_FireMode.Num(); i++) {
	// 	// 	UTgDeviceFire* FireMode = Jetpack->m_FireMode.Data[i];
	// 	// 	Jetpack->CurrentFireMode = FireMode->m_nFireType;
	// 	// }
	// 	// Jetpack->CurrentFireMode = 2;
	//
	// 	Jetpack->r_nInventoryId = nMaxInventoryId + 2;
	// 	Jetpack->s_InventoryObject->m_InventoryData.nInvId = nMaxInventoryId + 2;
	//
	// 	if ((char*)newpawn->InvManager + 0x1f0 == nullptr) {
	// 		TMap__Allocate::CallOriginal((void*)((char*)newpawn->InvManager + 0x1f0));
	// 	}
	// 	TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)newpawn->InvManager + 0x1f0), edx, nMaxInventoryId + 2, Jetpack->s_InventoryObject);
	// 	ATgDevice* JetpackFixed = newpawn->CreateEquipDevice(nMaxInventoryId + 2, GA_G::DEVICE_ID_MEDIC_CJP, GA_G::EQUIP_POINT_JETPACK_ID);
	// 	if (JetpackFixed != nullptr) {
	// 		Logger::Log("debug", "JetpackFixed = 0x%p, %s\n", JetpackFixed, JetpackFixed->GetFullName());
	// 	} else {
	// 		Logger::Log("debug", "JetpackFixed = nullptr\n");
	// 	}
	//
	// 	// Logger::DumpMemory("jetpack", JetpackFixed, 0x2D8, 0);
	//
	// 	Agonizer->r_nInventoryId = nMaxInventoryId + 1;
	// 	Agonizer->s_InventoryObject->m_InventoryData.nInvId =nMaxInventoryId + 1;
	// 	TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)newpawn->InvManager + 0x1f0), edx, nMaxInventoryId + 1, Agonizer->s_InventoryObject);
	//
	// 	// newpawn->ForceUpdateEquippedDevices();
	// 	// newpawn->UpdateClientDevices();
	// } else {
	// 	Logger::Log("debug", "Jetpack = nullptr\n");
	// }

	newpawn->r_nPhysicalType = 860;
	newpawn->ReplicatedCollisionType = newpawn->CollisionType;
	// HP and power pool are initialized by InitializeDefaultProps from
	// asm_data_set_bots. r_nHealthMaximum is the post-init source of truth
	// (Engine APawn::HealthMax isn't written by SetProperty(304)).
	int hp = newpawn->r_nHealthMaximum;
	newpawn->r_nProfileId = classConfig.profileId;
	newpawn->r_bDisableAllDevices = 0;
	newpawn->r_bEnableEquip = 1;
	newpawn->r_bEnableSkills = 1;
	newpawn->r_bEnableCrafting = 1;
	newpawn->r_bIsStealthed = 0;
	newpawn->r_bIsBot = 0;
	newpawn->r_bIsDecoy = 0;
	newpawn->r_bIsHacking = 0;
	newpawn->r_fCurrentPowerPool = newpawn->r_fMaxPowerPool;  // set by SetProperty(TGPID_POWERPOOL_MAX)
	newpawn->r_nXp = 999999;

	newpawn->r_EffectManager->r_Owner = newpawn;
	newpawn->r_EffectManager->SetOwner(newpawn);
	newpawn->r_EffectManager->Base = newpawn;
	newpawn->r_EffectManager->Role = 3;

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
	newpawn->r_nSkillGroupSetId = classConfig.skillGroupSetId;
	newpawn->s_nCharacterId = (int)GClientConnectionsData[ConnectionIndex].PlayerInfo.selected_character_id;



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
	newrepplayer->r_nProfileId = classConfig.profileId;
	// PRI is now wired to newpawn — fan HP across all 7 storage locations
	// (engine fields + property descriptors + PRI replicated fields).
	SyncPawnHealth::Apply(newpawn, hp, hp);
	newrepplayer->r_nCharacterId = newpawn->s_nCharacterId;
	newrepplayer->r_nLevel = 50;
	// newrepplayer->r_sOrigPlayerName = FString(L"Zaxik");
	newrepplayer->r_PawnOwner = newpawn;
	newrepplayer->r_ApproxLocation = newpawn->Location;
	// newrepplayer->PlayerName = FString(L"Zaxik");


	// eventSetPlayerName passes FString to ProcessEvent via memcpy (shallow copy).
	// ProcessEvent's cleanup calls appFree on the FString::Data (CPF_NeedCtorLink).
	// If Data was allocated with C++ new[], appFree corrupts the heap.
	// Null out Data after the memcpy inside eventSetPlayerName copies it,
	// so the C++ destructor doesn't also delete[] the same pointer.
	{
		FString playerNameStr(GClientConnectionsData[(int)PlayerController->Player].PlayerInfo.player_name_w.data());
		newrepplayer->eventSetPlayerName(playerNameStr);
		// eventSetPlayerName memcpy'd our FString into its Parms struct.
		// ProcessEvent already consumed (and potentially freed) the Data.
		// Null out so ~FString doesn't double-free.
		playerNameStr.Data = nullptr;
		playerNameStr.Count = playerNameStr.Max = 0;
	}
	// newrepplayer->SetTeam(GTeamsData.Attackers);

	// newrepplayer->r_TaskForce = GTeamsData.Defenders;
	// newrepplayer->Team = GTeamsData.Defenders;
	// newrepplayer->SetTeam(GTeamsData.Defenders);

	{
		int tf = GClientConnectionsData[ConnectionIndex].PlayerInfo.task_force;
		ATgRepInfo_TaskForce* taskforce = (tf == 1) ? GTeamsData.Attackers : GTeamsData.Defenders;
		newrepplayer->r_TaskForce = taskforce;
		newrepplayer->Team = taskforce;
		newrepplayer->SetTeam(taskforce);
		newpawn->NotifyTeamChanged();
		Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: assigned to task_force=%d\n", tf);
	}

	// newrepplayer->r_TaskForce = GTeamsData.Attackers;
	// newrepplayer->SetTeam(GTeamsData.Attackers);

	// newpawn->NotifyTeamChanged();

	// newrepplayer->SetTeam(GTeamsData.Defenders);
	// newrepplayer->SetPlayerTeam(GTeamsData.Attackers);
	// newrepplayer->Team = GTeamsData.Attackers;
	newrepplayer->bNetDirty = 1;
	newrepplayer->bForceNetUpdate = 1;
	newrepplayer->bSkipActorPropertyReplication = 0;
	newrepplayer->bOnlyDirtyReplication = 0;
	newrepplayer->bNetInitial = 1;

	// if (PlayerController->myHUD != nullptr) {
	// 	PlayerController->myHUD->Destroy();
	// 	PlayerController->myHUD = nullptr;
	// }
	// APlayerController* AController = reinterpret_cast<APlayerController*>(PlayerController);
	// AController->ClientSetHUD(ClassPreloader::GetTgHudTeamGameClass(), Game->ScoreBoardType);
	// Logger::Log("debug", "Client HUD changed");

	// PlayerController->ClientShowHUDElement(5); // MissionInfo
	// PlayerController->ClientShowHUDElement(7); // TeamInfo
	// PlayerController->ClientHideHUDElement(10); // AgentInfo
	// PlayerController->ClientHideHUDElement(11); // QuestTracking 

	newpawn->r_UIClockTime = 15 * 60;
	newpawn->r_UIClockState = 0x00;

	// PlayerController->ViewTarget = newpawn;
	// if (PlayerController->PlayerCamera != nullptr) {
	// 	PlayerController->PlayerCamera->ViewTarget.Controller = PlayerController;
	// 	PlayerController->PlayerCamera->ViewTarget.PRI = newrepplayer;
	// 	PlayerController->PlayerCamera->ViewTarget.Target = newpawn;
	// }
	// PlayerController->RealViewTarget = newrepplayer;

	ATgRepInfo_TaskForce* attackers = GTeamsData.Attackers;
	ATgRepInfo_TaskForce* defenders = GTeamsData.Defenders;

	FTGTEAM_ENTRY newplayerteamentry;
	newplayerteamentry.fsName = FString(GClientConnectionsData[(int32_t)((UNetConnection*)PlayerController->Player)].PlayerInfo.player_name_w.data());
	newplayerteamentry.fsMapName = FString(L"Tetra Pier");
	newplayerteamentry.nHealth = hp;
	newplayerteamentry.nMaxHealth = hp;
	newplayerteamentry.bLeader = 0;
	newplayerteamentry.pPrep = newrepplayer;

	TARRAY_INIT(attackers, TeamPlayersAttackers, FTGTEAM_ENTRY, 0x214, 32);
	TARRAY_INIT(defenders, TeamPlayersDefenders, FTGTEAM_ENTRY, 0x214, 32);

	{
		int tf = GClientConnectionsData[ConnectionIndex].PlayerInfo.task_force;
		if (tf == 1) {
			TARRAY_ADD(TeamPlayersAttackers, newplayerteamentry);
		} else {
			TARRAY_ADD(TeamPlayersDefenders, newplayerteamentry);
		}
	}

	// TARRAY_ADD shallow-copies the struct (memcpy-style).  The TArray copy
	// now owns the FString Data pointers.  Null them in the local so
	// ~FString doesn't delete[] memory that UE3 will later appFree.
	newplayerteamentry.fsName.Data = nullptr;
	newplayerteamentry.fsName.Count = newplayerteamentry.fsName.Max = 0;
	newplayerteamentry.fsMapName.Data = nullptr;
	newplayerteamentry.fsMapName.Count = newplayerteamentry.fsMapName.Max = 0;

	// GIVE COLONY EYE PET

	// FVector PetSpawnLocation = newpawn->Location;
	// PetSpawnLocation.X += 100;
	// ATgPawn_VanityPet* Pet = (ATgPawn_VanityPet*)Game->SpawnBotById(1657, PetSpawnLocation, newpawn->Rotation, false, nullptr, true, newpawn, false, nullptr, 0.0f);
	// if (Pet != nullptr) {
	// 	newpawn->r_CurrentVanityPet = Pet;
	// }

	// OLD: TgGame__SpawnBotById::GiveDevicesFromBotConfig(newpawn, newrepplayer, PLAYER_BOT_ID);

	Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: profileId=%d, skillGroupSetId=%d, botId=%d, characterId=%d, hp=%d\n",
	    classConfig.profileId, classConfig.skillGroupSetId, profileId, newpawn->s_nCharacterId, hp);

	// Read equipped devices from DB — single source of truth shared with control server
	{
		int64_t charId = (int64_t)newpawn->s_nCharacterId;
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT device_id, equip_slot, quality, inventory_id "
			"FROM ga_character_devices WHERE character_id = ? ORDER BY equip_slot",
			-1, &stmt, nullptr);
		if (rc == SQLITE_OK && stmt) {
			sqlite3_bind_int64(stmt, 1, charId);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				int deviceId    = sqlite3_column_int(stmt, 0);
				int slot        = sqlite3_column_int(stmt, 1);
				int quality     = sqlite3_column_int(stmt, 2);
				int inventoryId = sqlite3_column_int(stmt, 3);
				Inventory::Equip(newpawn, deviceId, slot, quality, inventoryId);
			}
			sqlite3_finalize(stmt);
		} else {
			Logger::Log(GetLogChannel(), "SpawnPlayerCharacter: failed to query ga_character_devices for charId=%lld: %s\n",
				charId, sqlite3_errmsg(db));
		}
		Inventory::Finalize(newpawn);
	}

	// NOTE: we do NOT call ReapplyCharacterSkillTree here — GPawnSessions is
	// only populated later in MarshalChannel__NotifyControlMessage::
	// HandlePlayerConnected, and our Reapply hook looks up the pawn's session
	// guid there to resolve PlayerInfo.skills. Calling it at this stage logs
	// "pawn has no session mapping" and silently no-ops. The hook is invoked
	// from HandlePlayerConnected instead (right after GPawnSessions[pawn] is
	// set), so the skill data is reachable.

	LogCallEnd();

	return newpawn;
}

