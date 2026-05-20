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
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
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

	// r_nPawnId is assigned by UC TgPawn.PostBeginPlay via TgGame.GetNextPawnId()
	// during Spawn() — per-TgGame-instance monotonic counter. Don't override.
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

	// Force ROLE_Authority on the server-spawned player pawn, mirroring every
	// other server-spawn path (SpawnBotById/SpawnNextBot/SpawnDeployable/
	// SpawnPet all set Role=3). UWorld::SpawnActor swaps Role<->RemoteRole when
	// bRemoteOwned=true (PlayerController-owned spawns), which would land us
	// at Role=ROLE_AutonomousProxy(2). UC gates ProcessInstantHit on
	// `Instigator.Role == ROLE_Authority` (TgDevice.uc:1593), so without this
	// every player-fired hitscan/repair impact gets dropped before ApplyHit.
	newpawn->Role       = 3;  // ROLE_Authority
	newpawn->RemoteRole = 1;  // ROLE_SimulatedProxy

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

	// Read equipped devices from DB.
	//
	// Schema since v53: ga_character_devices is a thin (character_id,
	// inventory_id, equipped_slot) join row; the actual device descriptor
	// (device_id / quality / mods / oc) lives in the account-scoped
	// ga_players_inventory pool, keyed by inventory_id. JOIN here so the
	// in-engine equip path keeps getting the same (device, slot, mods)
	// tuples it did before, just sourced from two tables instead of one.
	//
	// Also auto-attach the profile's class device (slot 14, the rest /
	// HUMAN BASE ATTRIBUTES) from asm_data_set_bots_data_set_bot_devices
	// — that one is not player-equippable so it lives outside the
	// inventory pool and gets re-applied unconditionally on every spawn.
	{
		int64_t charId = (int64_t)newpawn->s_nCharacterId;
		sqlite3* db = Database::GetConnection();

		// (1) Player-chosen equipped gear.
		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT i.device_id, d.equipped_slot, i.quality, i.id, i.mod_effect_group_ids "
			"FROM ga_character_devices d "
			"JOIN ga_players_inventory i ON i.id = d.inventory_id "
			"WHERE d.character_id = ? "
			"ORDER BY d.equipped_slot",
			-1, &stmt, nullptr);
		if (rc == SQLITE_OK && stmt) {
			sqlite3_bind_int64(stmt, 1, charId);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				int deviceId    = sqlite3_column_int(stmt, 0);
				int slot        = sqlite3_column_int(stmt, 1);
				int quality     = sqlite3_column_int(stmt, 2);
				int inventoryId = sqlite3_column_int(stmt, 3);

				// Parse mod_effect_group_ids CSV ("24208,24211,24212,...") into ints.
				std::vector<int> mods;
				const char* csv = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
				if (csv && *csv) {
					const char* p = csv;
					while (*p) {
						while (*p == ',' || *p == ' ') ++p;
						if (!*p) break;
						char* end = nullptr;
						long v = std::strtol(p, &end, 10);
						if (end == p) break;
						mods.push_back((int)v);
						p = end;
					}
				}

				Inventory::Equip(newpawn, deviceId, slot, quality, inventoryId, mods);
			}
			sqlite3_finalize(stmt);
		} else {
			Logger::Log(GetLogChannel(),
				"SpawnPlayerCharacter: failed to query ga_character_devices for charId=%lld: %s\n",
				charId, sqlite3_errmsg(db));
		}

		// Class device (slot 14 / HUMAN BASE ATTRIBUTES) used to be attached
		// here via a synthetic invId outside the inventory pool. That
		// corrupted the inventory TMap on disconnect cleanup (three
		// FMallocWindows::Free crash conditions detected). It's now a regular
		// inventory row: SeedInventoryFromLoadouts adds it to
		// ga_players_inventory, the opening-loadout pass equips it in slot 14
		// on character creation, and SaveEquippedDevices pins it to slot 14
		// on every equip save. So the JOIN query above just returns it like
		// any other equipped device — no special handling needed here.

		Inventory::Finalize(newpawn);

		// r_ItemCount (ATgInventoryManager+0x1e8, CPF_Net) MUST equal the count
		// of non-null entries in m_InventoryMap on the client, otherwise
		// ATgInventoryManager::IsValid() @ 0x10a163f0 returns false. That gates
		// UTgUIAgentProfile_Equip::FixupWidgets's call to FUN_11416a20 — the
		// function that binds invObj → slot widget. Result of mismatch: equip
		// screen renders all slot widgets blank even though m_EquippedDevices
		// is populated and the device bar works.
		//
		// Inventory::Equip increments r_ItemCount by 1 per equipped device,
		// but send_inventory_response on the control server also ships the
		// account-scoped bag pool. The client adds every record to its
		// m_InventoryMap, so its count = equipped + bag. We need r_ItemCount
		// to match the same total. Query the DB for the full pool size.
		{
			sqlite3_stmt* who = nullptr;
			int rc = sqlite3_prepare_v2(db,
				"SELECT user_id, profile_id FROM ga_characters WHERE id = ?",
				-1, &who, nullptr);
			if (rc == SQLITE_OK && who) {
				sqlite3_bind_int64(who, 1, charId);
				if (sqlite3_step(who) == SQLITE_ROW) {
					int64_t  user_id    = sqlite3_column_int64(who, 0);
					int      profile_id = sqlite3_column_int  (who, 1);

					sqlite3_stmt* cnt = nullptr;
					rc = sqlite3_prepare_v2(db,
						"SELECT COUNT(*) FROM ga_players_inventory "
						"WHERE user_id = ? AND (profile_id = 0 OR profile_id = ?)",
						-1, &cnt, nullptr);
					if (rc == SQLITE_OK && cnt) {
						sqlite3_bind_int64(cnt, 1, user_id);
						sqlite3_bind_int  (cnt, 2, profile_id);
						if (sqlite3_step(cnt) == SQLITE_ROW) {
							const int total = sqlite3_column_int(cnt, 0);
							((ATgInventoryManager*)newpawn->InvManager)->r_ItemCount = total;
							Logger::Log(GetLogChannel(),
								"SpawnPlayerCharacter: InvManager->r_ItemCount=%d (pool total, IsValid gate)\n",
								total);
						}
						sqlite3_finalize(cnt);
					}
				}
				sqlite3_finalize(who);
			}
		}
	}

	// Force a post-spawn transform refresh. TgGame.RestartPlayer's bHadPawn
	// branch (TgGame.uc:558-559) does this on every respawn but NOT on the
	// first spawn — and without it, server-side hitscan traces fire from a
	// position that tracks the player but is angularly displaced, making
	// aiming nearly impossible until the player dies and respawns.
	// SetLocation/SetRotation force ConditionalUpdateComponents on the pawn,
	// which re-resolves component-relative transforms (collision cylinder,
	// skeletal mesh, attached weapon meshes) against the now-correct
	// Location/Rotation. Mirrors the respawn fix-up so first spawn matches.
	newpawn->SetLocation(SpawnLocation);
	newpawn->SetRotation(PlayerController->Rotation);

	// Update GPawnSessions and reapply skills. SpawnPlayerCharacter fires both
	// on INITIAL connect AND on RESPAWN — but HandlePlayerConnected (which used
	// to be the only Reapply call site) only fires on initial connect, so
	// without doing it here, skill-based effect groups like Fast Regeneration
	// (skill 902, +80HP/2s passive heal-over-time) die with the previous pawn
	// and never re-attach to the respawn pawn.
	//
	// On initial connect SessionGuid might not be set yet (PLAYER_REGISTER
	// IPC arrives async). In that case skip — HandlePlayerConnected:303 will
	// do the mapping + Reapply once SessionGuid lands. On respawn SessionGuid
	// is already populated from the original connection.
	const std::string& session_guid = GClientConnectionsData[ConnectionIndex].SessionGuid;
	if (!session_guid.empty()) {
		// Drop any stale mapping from this player's previous pawn (death
		// cycle). GPawnSessions is keyed by pawn pointer — the dead pawn is
		// still in the map but no longer relevant. NetConnection::Cleanup
		// erases it on disconnect, but respawn doesn't trigger that path.
		for (auto it = GPawnSessions.begin(); it != GPawnSessions.end(); ) {
			if (it->second == session_guid && it->first != (ATgPawn*)newpawn) {
				it = GPawnSessions.erase(it);
			} else {
				++it;
			}
		}
		GPawnSessions[(ATgPawn*)newpawn] = session_guid;

		Logger::Log(GetLogChannel(),
			"SpawnPlayerCharacter: bound newpawn=%p to session=%s; calling ReapplyCharacterSkillTree\n",
			newpawn, session_guid.c_str());
		newpawn->ReapplyCharacterSkillTree();
	}

	LogCallEnd();

	return newpawn;
}

