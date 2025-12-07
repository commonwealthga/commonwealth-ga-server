#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/CreateDevice/AssemblyDatManager__CreateDevice.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

bool TgGame__SpawnPlayerCharacter::bEnemyGearSpawned = true; // disable temporarily because of lag

ATgPawn_Character* __fastcall TgGame__SpawnPlayerCharacter::Call(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation) {

	LogCallBegin();


	static int nMaxInventoryId = 1;

	//////////////////// SPAWN ENEMY BOTS EQUIPMENT START

	if (!bEnemyGearSpawned) {
		bEnemyGearSpawned = true;
		for (int i = 0; i < UObject::GObjObjects()->Num(); i++) {
			UObject* obj = UObject::GObjObjects()->Data[i];
			if (!obj) {
				continue;
			}

			if (TgGame__SpawnBotById::m_spawnedBotIds.find((int)obj) != TgGame__SpawnBotById::m_spawnedBotIds.end()) {
				ATgPawn_Character* Bot = reinterpret_cast<ATgPawn_Character*>(obj);
				ATgAIController* AIController = (ATgAIController*)Bot->Controller;
				ATgRepInfo_Player* BotRepInfo = reinterpret_cast<ATgRepInfo_Player*>(AIController->PlayerReplicationInfo);
				int nBotId = TgGame__SpawnBotById::m_spawnedBotIds[(int)Bot];

				if (Bot->InvManager == nullptr) {
					Logger::Log("debug", "Bot %d has no inventory manager\n", nBotId);
					continue;
				}


				sqlite3* db = Database::GetConnection();
				sqlite3_stmt* stmt;
				int result = sqlite3_prepare_v2(db, "SELECT \
					b.bot_id AS bot_id, \
					bd.slot_used_value_id AS slot_used_value_id, \
					bd.device_id AS device_id, \
					i.quality_value_id AS quality_value_id \
					MIN(m.device_mode_id) AS device_mode_id, \
					b.default_slot_value_id AS default_slot_value_id \
					FROM asm_data_set_bots_data_set_bot_devices bd \
					LEFT JOIN asm_data_set_bots b ON b.bot_id = bd.bot_id \
					LEFT JOIN asm_data_set_items i ON i.item_id = bd.device_id \
					LEFT JOIN asm_data_set_devices_data_set_device_modes m ON m.device_id = bd.device_id \
					WHERE b.bot_id = ? \
					GROUP BY b.bot_id, bd.device_id;", -1, &stmt, nullptr);
				if (result == SQLITE_OK) {
					sqlite3_bind_int(stmt, 1, nBotId);

					while (sqlite3_step(stmt) == SQLITE_ROW) {

						nMaxInventoryId++;
						int botId = sqlite3_column_int(stmt, 0);
						int slotUsedValueId = sqlite3_column_int(stmt, 1);
						int deviceId = sqlite3_column_int(stmt, 2);
						int qualityValueId = sqlite3_column_int(stmt, 3);
						int deviceModeId = sqlite3_column_int(stmt, 4);
						int defaultSlotValueId = sqlite3_column_int(stmt, 5);
						if (deviceId == 0) {
							continue;
						}

						Logger::Log("debug", "Requested new device: botId = %d, slotUsedValueId = %d, deviceId = %d, qualityValueId = %d\n", botId, slotUsedValueId, deviceId, qualityValueId);

						// int equipPoint = Bot->GetEquipPointByType(slotUsedValueId);

						int equipPoint = GetEquipPointByType(slotUsedValueId);

						Logger::Log("debug", "slotUsedValueId = %d, equipPoint = %d\n", slotUsedValueId, equipPoint);

						UTgInventoryObject_Device* InventoryObject = (UTgInventoryObject_Device*)TgInventoryObject_Device__ConstructInventoryObject::CallOriginal(
							ClassPreloader::GetTgInventoryObjectDeviceClass(),
							-1, 0, 0, 0, 0, 0, 0, nullptr);
						// add RF_RootSet flag 0x4000
						InventoryObject->ObjectFlags.A |= 0x4000;


						Logger::Log("debug", "Created inventory object %s\n", InventoryObject->GetFullName());

						FInventoryData InventoryData;
						InventoryData.bBoundFlag = 0;
						InventoryData.bEquippedOnOtherChar = 0;
						InventoryData.nCreatedByCharacterId = 0;
						InventoryData.fAcquiredDatetime = 0.0f;
						InventoryData.m_nEquipSlotValueIdArray[0] = slotUsedValueId;
						InventoryData.nCraftedQualityValueId = qualityValueId;
						InventoryData.nBlueprintId = 0;
						InventoryData.nDurability = 100;
						InventoryData.nInstanceCount = 1;
						InventoryData.nInvId = nMaxInventoryId;
						InventoryData.nItemId = deviceId;
						InventoryData.nLocationCode = 370;

						InventoryObject->m_pAmItem.Dummy = CAmItem__LoadItemMarshal::m_ItemPointers[deviceId];
						InventoryObject->m_InventoryData = InventoryData;
						InventoryObject->s_bPersistsInInventory = 0;
						InventoryObject->s_ReplicatedState = 0;

						InventoryObject->m_InvManager = (ATgInventoryManager*)Bot->InvManager;
						ATgDevice* Device = AssemblyDatManager__CreateDevice::CallOriginal(Globals::Get().GAssemblyDatManager, edx, InventoryObject->m_InventoryData.nItemId, Bot);
						InventoryObject->s_Device = Device;

						if (Device != nullptr) {
							Device->s_InventoryObject = InventoryObject;
							Device->r_nDeviceInstanceId = nMaxInventoryId;

							FEquipDeviceInfo EquipDeviceInfo;
							EquipDeviceInfo.nDeviceId = InventoryObject->m_InventoryData.nItemId;
							EquipDeviceInfo.nDeviceInstanceId = nMaxInventoryId;
							EquipDeviceInfo.nQualityValueId = InventoryObject->m_InventoryData.nCraftedQualityValueId;

							// UTgDeviceForm* DeviceForm = Bot->CreateDeviceForm(EquipDeviceInfo);
							// Bot->c_EquipForm[equipPoint] = DeviceForm;

							Bot->m_EquippedDevices[equipPoint] = Device;

							Bot->r_EquipDeviceInfo[equipPoint] = EquipDeviceInfo;

							BotRepInfo->r_EquipDeviceInfo[equipPoint] = EquipDeviceInfo;

							Device->r_eEquippedAt = equipPoint;
							Device->r_nInventoryId = nMaxInventoryId;
							Device->s_InventoryObject->m_InventoryData.nInvId = nMaxInventoryId;

							
							if ((char*)Bot->InvManager + 0x1f0 == nullptr) {
								TMap__Allocate::CallOriginal((void*)((char*)Bot->InvManager + 0x1f0));
							}
							if ((char*)Bot->InvManager + 0x22c == nullptr) {
								TMap__Allocate::CallOriginal((void*)((char*)Bot->InvManager + 0x22c));
							}
							TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)Bot->InvManager + 0x1f0), edx, Device->s_InventoryObject->m_InventoryData.nInvId, Device->s_InventoryObject);
							TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)Bot->InvManager + 0x22c), edx, Device->s_InventoryObject->m_InventoryData.nInvId, Device->s_InventoryObject);
							
							// Device = Bot->CreateEquipDevice(nMaxInventoryId, deviceId, equipPoint);


							Device->SetOwner(Bot);
							Device->Base = Bot;
							Device->Role = 3;
							Device->RemoteRole = 1;
							Device->bNetInitial = 1;
							Device->bNetDirty = 1;
							Device->bReplicateMovement = 1;
							Device->bSkipActorPropertyReplication = 0;
							Device->bOnlyDirtyReplication = 0;
							Device->bAlwaysRelevant = 0;

							if (defaultSlotValueId == slotUsedValueId) {
								// Bot->Weapon = Device;
								Bot->r_eDesiredInHand = equipPoint;
								// ATgInventoryManager* InventoryManager = (ATgInventoryManager*)Bot->InvManager;
								// AIController->m_eEquipmentSlot = equipPoint;
								// AIController->m_nDeviceMode = deviceModeId;
								// InventoryManager->SetCurrentWeapon(Device, 0, 0, 0);
							}
							// Bot->ForceUpdateEquippedDevices();

							Logger::Log("debug", "Device created successfully %p\n", Device);
						} else {
							Logger::Log("debug", "Device not created\n");
						}


						// Logger::DumpMemory("inventoryobject", InventoryObject, 0xBC, 0);


					}
				}

			}
		}
	}
	//////////////////// SPAWN ENEMY BOTS EQUIPMENT END

	ATgPawn_Character* newpawn = (ATgPawn_Character*)Game->Spawn(ClassPreloader::GetTgPawnCharacterClass(), PlayerController, FName(), SpawnLocation, PlayerController->Rotation, nullptr, 1);
	newpawn->r_nPawnId = 998;
	ATgRepInfo_Player* newrepplayer = reinterpret_cast<ATgRepInfo_Player*>(PlayerController->PlayerReplicationInfo);

	PlayerController->Pawn = newpawn;
	newpawn->Controller = PlayerController;

	nMaxInventoryId++;






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
	newpawn->r_nHealthMaximum = 1300;
	newpawn->r_nProfileId = 567; // medic
	newpawn->r_bDisableAllDevices = 0;
	newpawn->r_bEnableEquip = 1;
	newpawn->r_bEnableSkills = 1;
	newpawn->r_bEnableCrafting = 1;
	newpawn->r_bIsStealthed = 0;
	newpawn->r_bIsBot = 0;
	newpawn->r_bIsDecoy = 0;
	newpawn->r_bIsHacking = 0;
	newpawn->r_fCurrentPowerPool = 100;
	newpawn->r_fMaxPowerPool = 100;
	newpawn->r_nXp = 999999;
	newpawn->Health = 1300;
	newpawn->HealthMax = 1300;

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
	newpawn->r_nSkillGroupSetId = GA_G::SKILL_GROUP_SET_ID_MEDIC;
	newpawn->s_nCharacterId = 373;



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

	// newrepplayer->r_TaskForce = GTeamsData.Defenders;
	// newrepplayer->Team = GTeamsData.Defenders;
	// newrepplayer->SetTeam(GTeamsData.Defenders);

	newrepplayer->r_TaskForce = GTeamsData.Attackers;
	newrepplayer->Team = GTeamsData.Attackers;
	newrepplayer->SetTeam(GTeamsData.Attackers);

	newpawn->NotifyTeamChanged();

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

	newpawn->r_UIClockState = 0;
	newpawn->r_UIClockTime = 15 * 60;

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
	newplayerteamentry.fsName = FString(L"Zaxik");
	newplayerteamentry.fsMapName = FString(L"Tetra Pier");
	newplayerteamentry.nHealth = 1300;
	newplayerteamentry.nMaxHealth = 1300;
	newplayerteamentry.bLeader = 0;
	newplayerteamentry.pPrep = newrepplayer;

	TARRAY_INIT(attackers, TeamPlayersAttackers, FTGTEAM_ENTRY, 0x214, 32);
	TARRAY_INIT(defenders, TeamPlayersDefenders, FTGTEAM_ENTRY, 0x214, 32);

	TARRAY_ADD(TeamPlayersAttackers, newplayerteamentry);
	// TARRAY_ADD(TeamPlayersDefenders, newplayerteamentry);

	LogCallEnd();

	return newpawn;
}

