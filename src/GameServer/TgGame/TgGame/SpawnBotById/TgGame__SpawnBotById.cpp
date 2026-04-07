#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/TgGame/TgAIController/InitBehavior/TgAIController__InitBehavior.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/CreateDevice/AssemblyDatManager__CreateDevice.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

std::map<int, int> TgGame__SpawnBotById::m_spawnedBotIds;

static inline int BotGetEquipPointBySlot(int slotUsedValueId) {
	switch (slotUsedValueId) {
		case 221: return 1;  case 198: return 2;  case 200: return 3;
		case 199: return 4;  case 201: return 5;  case 202: return 6;
		case 203: return 7;  case 204: return 8;  case 385: return 9;
		case 386: return 10; case 499: return 11; case 500: return 12;
		case 501: return 13; case 502: return 14; case 823: return 15;
		case 996: return 16; case 997: return 17; case 998: return 18;
		case 999: return 19; case 1000: return 20; case 1001: return 21;
		case 1002: return 22; case 1003: return 23; case 1004: return 24;
		default:  return 0;
	}
}

void TgGame__SpawnBotById::GiveDeviceById(
	ATgPawn* Pawn,
	ATgRepInfo_Player* PlayerReplicationInfo,
	int deviceId,
	int slotUsedValueId,
	int equipPoint,
	int qualityValueId,
	bool isOffHand,
	bool isHandDevice,
	int deviceType,
	int nInventoryId
) {
	// int equipPoint = BotGetEquipPointBySlot(slotUsedValueId);
	

	UTgInventoryObject_Device* InventoryObject = (UTgInventoryObject_Device*)TgInventoryObject_Device__ConstructInventoryObject::CallOriginal(
		ClassPreloader::GetTgInventoryObjectDeviceClass(),
		-1, 0, 0, 0, 0, 0, 0, nullptr);
	// add RF_RootSet flag 0x4000
	InventoryObject->ObjectFlags.A |= 0x4000;

	FInventoryData InventoryData;
	InventoryData.bBoundFlag = 1;
	InventoryData.bEquippedOnOtherChar = 0;
	InventoryData.nCreatedByCharacterId = 998;
	InventoryData.fAcquiredDatetime = 1700000000.0f;
	// 198 = ranged weapon slot value ID, maps to equip point 2 via FUN_109a1320
	InventoryData.m_nEquipSlotValueIdArray[0] = slotUsedValueId;
	InventoryData.m_nEquipSlotValueIdArray[1] = slotUsedValueId;
	InventoryData.m_nEquipSlotValueIdArray[2] = slotUsedValueId;
	InventoryData.m_nEquipSlotValueIdArray[3] = slotUsedValueId;
	InventoryData.m_nEquipSlotValueIdArray[4] = slotUsedValueId;
	InventoryData.m_nEquipSlotValueIdArray[5] = slotUsedValueId;
	InventoryData.nCraftedQualityValueId = 1162;
	InventoryData.nBlueprintId = 0;
	InventoryData.nDurability = 100;
	InventoryData.nInstanceCount = 1;
	InventoryData.nInvId = nInventoryId;
	InventoryData.nItemId = deviceId;
	InventoryData.nLocationCode = 369;

	InventoryObject->m_pAmItem.Dummy = CAmItem__LoadItemMarshal::m_ItemPointers[deviceId];
	InventoryObject->m_InventoryData = InventoryData;
	InventoryObject->s_bPersistsInInventory = 0;
	InventoryObject->s_ReplicatedState = 1;
	InventoryObject->m_nDeviceInstanceId = nInventoryId;

	InventoryObject->m_InvManager = (ATgInventoryManager*)Pawn->InvManager;

	// Add to m_InventoryMap only (not s_ReplicateMap — normal flow only uses 0x1F0)
	TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)Pawn->InvManager + 0x1f0), nullptr, nInventoryId, InventoryObject);
	// Keep r_ItemCount in sync with map entry count
	((ATgInventoryManager*)Pawn->InvManager)->r_ItemCount++;
	

	ATgDevice* Device = Pawn->CreateEquipDevice(nInventoryId, deviceId, equipPoint);

	InventoryObject->s_Device = Device;

	if (Device != nullptr) {
		Device->s_InventoryObject = InventoryObject;
		Device->r_nDeviceInstanceId = nInventoryId;  // non-zero so UpdateClientDevices detects the change
		Device->m_bIsOffHand = isOffHand;
		Device->m_bHandDevice = isHandDevice;
		Device->m_nDeviceType = deviceType;
		Device->r_nDeviceId = deviceId;
		Device->r_nQualityValueId = qualityValueId;

		FEquipDeviceInfo EquipDeviceInfo;
		EquipDeviceInfo.nDeviceId = deviceId;
		EquipDeviceInfo.nDeviceInstanceId = nInventoryId;
		EquipDeviceInfo.nQualityValueId = InventoryObject->m_InventoryData.nCraftedQualityValueId;

		Pawn->m_EquippedDevices[equipPoint] = Device;

		// NOTE: Do NOT set Pawn->r_EquipDeviceInfo[equipPoint] here.
		// CreateEquipDevice already wrote r_EquipDeviceInfo[equipPoint].nDeviceInstanceId = 0
		// (from device->r_nDeviceInstanceId which was 0 at that point). Now that we've set
		// Device->r_nDeviceInstanceId = nInventoryId, UpdateClientDevices will detect the mismatch
		// and both (a) update r_EquipDeviceInfo from device fields and (b) fire the equip handler
		// + ProcessEvent notification that drives client-side device bar update.
		// If we pre-set r_EquipDeviceInfo here, UpdateClientDevices sees equality → no-op → client never notified.
		PlayerReplicationInfo->r_EquipDeviceInfo[equipPoint] = EquipDeviceInfo;

		Device->r_eEquippedAt = equipPoint;
		if (isHandDevice) Pawn->r_eDesiredInHand = equipPoint;
		Device->r_nInventoryId = nInventoryId;
		Device->s_InventoryObject->m_InventoryData.nInvId = nInventoryId;

		Device->Instigator = (APawn*)Pawn;

		Device->Base = Pawn;
		Device->Role = 3;
		Device->RemoteRole = 1;
		Device->bNetInitial = 1;
		Device->bNetDirty = 1;
		Device->bReplicateMovement = 1;
		Device->bSkipActorPropertyReplication = 0;
		Device->bOnlyDirtyReplication = 0;
		// Device->bAlwaysRelevant = 1;

		if (deviceId == 864) {
			*(uint32_t*)((char*)Device + 0x22C) |= 0x00020000;    // m_bIsRestDevice
			*(ATgDevice**)((char*)Pawn + 0x0904) = Device;         // m_RestDevice
			*(int*)((char*)Pawn + 0x1524) = equipPoint;             // r_nRestDeviceSlot (replicated)
		}
		if (deviceType == GA_G::TGDT_MORALE) {
			*(int*)((char*)Pawn + 0x1520) = equipPoint;             // r_nMoraleDeviceSlot (replicated)
		}

		// UpdateClientDevices detects: device->r_nDeviceInstanceId (nInventoryId) ≠ r_EquipDeviceInfo[slot] (0)
		// → updates r_EquipDeviceInfo from device, fires equip handler + ProcessEvent → client device bar update.
		Pawn->UpdateClientDevices(0, 0);

		Pawn->bNetDirty = 1;
		Pawn->bForceNetUpdate = 1;
	}
}

void TgGame__SpawnBotById::GiveDevicesFromBotConfig(ATgPawn* Bot, ATgRepInfo_Player* BotRepInfo, int nBotId) {
	Logger::Log("debug", "GiveDevicesFromBotConfig: botId=%d InvManager=0x%p\n", nBotId, Bot->InvManager);
	if (Bot->InvManager == nullptr) return;

	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* stmt;
	int result = sqlite3_prepare_v2(db,
		"SELECT DISTINCT bd.device_id, bd.slot_used_value_id, "
		"COALESCE(i.quality_value_id, 1162) AS quality_value_id, "
		"b.default_slot_value_id "
		"FROM asm_data_set_bots_data_set_bot_devices bd "
		"LEFT JOIN asm_data_set_bots b ON b.bot_id = bd.bot_id "
		"LEFT JOIN asm_data_set_items i ON i.item_id = bd.device_id "
		"WHERE bd.bot_id = ?;",
		-1, &stmt, nullptr);

	if (result != SQLITE_OK) return;

	sqlite3_bind_int(stmt, 1, nBotId);

	ATgDevice* primaryDevice = nullptr;
	int primaryEquipPoint = 0;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int deviceId           = sqlite3_column_int(stmt, 0);
		int slotUsedValueId    = sqlite3_column_int(stmt, 1);
		int qualityValueId     = sqlite3_column_int(stmt, 2);
		int defaultSlotValueId = sqlite3_column_int(stmt, 3);
		if (deviceId == 0) continue;

		int equipPoint = BotGetEquipPointBySlot(slotUsedValueId);
		Logger::Log("debug", "GiveDevicesFromBotConfig: deviceId=%d slotUsedValueId=%d equipPoint=%d\n", deviceId, slotUsedValueId, equipPoint);
		ATgDevice* Device = Bot->CreateEquipDevice(0, deviceId, equipPoint);
		Logger::Log("debug", "GiveDevicesFromBotConfig: CreateEquipDevice returned 0x%p\n", Device);
		if (Device != nullptr) {
			int invId = Inventory::NextId();
			Device->r_nDeviceInstanceId = invId;
			Device->r_eEquippedAt = equipPoint;
			// CreateEquipDevice leaves r_eEquippedAt=0 at creation time, so the device ends up
			// at m_EquippedDevices[0] and r_EquipDeviceInfo[0]. Fix both slot mappings.
			Bot->m_EquippedDevices[0] = nullptr;
			Bot->m_EquippedDevices[equipPoint] = Device;
			Bot->r_EquipDeviceInfo[0].nDeviceId = 0;
			Bot->r_EquipDeviceInfo[0].nDeviceInstanceId = 0;
			Bot->r_EquipDeviceInfo[0].nQualityValueId = 0;
			Bot->r_EquipDeviceInfo[equipPoint].nDeviceId = deviceId;
			Bot->r_EquipDeviceInfo[equipPoint].nDeviceInstanceId = invId;
			Bot->r_EquipDeviceInfo[equipPoint].nQualityValueId = qualityValueId;
			// Instigator must be set so TgDeviceFire uses Instigator.GetAimStart() for
			// projectile spawn location.
			Device->Instigator = (APawn*)Bot;
			Device->Role = 3;
			Device->RemoteRole = 1;
			Device->bNetInitial = 1;
			Device->bNetDirty = 1;
			BotRepInfo->r_EquipDeviceInfo[equipPoint].nDeviceId = deviceId;
			BotRepInfo->r_EquipDeviceInfo[equipPoint].nDeviceInstanceId = invId;
			BotRepInfo->r_EquipDeviceInfo[equipPoint].nQualityValueId = qualityValueId;

			// REST device: set the pawn-level REST tracking fields.
			// m_bIsRestDevice is bit 17 (0x00020000) of ATgDevice+0x22C.
			// m_RestDevice (ATgPawn+0x0904) is used by SetRestDevice/InterruptRestDevice.
			// r_nRestDeviceSlot (ATgPawn+0x1524, replicated) is used by InterruptRestDevice
			// to find the device via GetDeviceByEqPoint.
			// m_bIsRestDevice (bit 17 of +0x22C) is not set by CreateEquipDevice.
			// Detect REST devices by device ID and force-set the bit + pawn REST fields.
			if (deviceId == 864) {
				*(uint32_t*)((char*)Device + 0x22C) |= 0x00020000;    // m_bIsRestDevice
				*(ATgDevice**)((char*)Bot + 0x0904) = Device;         // m_RestDevice
				*(int*)((char*)Bot + 0x1524) = equipPoint;             // r_nRestDeviceSlot (replicated)
				Logger::Log("debug", "GiveDevicesFromBotConfig: REST device 864 at equipPoint=%d\n", equipPoint);
			}

			if (primaryDevice == nullptr || slotUsedValueId == defaultSlotValueId) {
				primaryDevice = Device;
				primaryEquipPoint = equipPoint;
			}
		}
	}
	sqlite3_finalize(stmt);

	if (primaryDevice != nullptr) {
		Bot->m_CurrentInHandDevice = primaryDevice;
		Bot->r_eDesiredInHand = primaryEquipPoint;
	}

	Bot->UpdateClientDevices(0, 0);
	Bot->bNetDirty = 1;
	Bot->bForceNetUpdate = 1;
	BotRepInfo->bNetDirty = 1;
	BotRepInfo->bForceNetUpdate = 1;
}


ATgPawn* __fastcall TgGame__SpawnBotById::Call(
	ATgGame* Game,
	void* edx,
	int nBotId,
	FVector vLocation,
	FRotator rRotation,
	bool bKillController,
	ATgBotFactory* pFactory,
	bool bIgnoreCollision,
	ATgPawn* pOwnerPawn,
	bool bIsDecoy,
	UTgDeviceFire* deviceFire,
	float fDeployAnimLength
) {
	Logger::Log("debug", "Spawning bot with id %d\n", nBotId);

	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);

	ATgAIController* AIController = (ATgAIController*)Game->Spawn(
		ClassPreloader::GetTgAIControllerClass(),
		WorldInfo,
		FName(),
		vLocation,
		rRotation,
		nullptr,
		bIgnoreCollision ? 1 : 0
	);

	if (AIController == nullptr) return nullptr;

	TgPawn__InitializeDefaultProps::nPendingBotId = nBotId;
	ATgPawn_Character* Bot = (ATgPawn_Character*)Game->Spawn(
		ClassPreloader::GetClass(GetPawnClassName(nBotId)),
		AIController->PlayerReplicationInfo,
		FName(),
		vLocation,
		rRotation,
		nullptr,
		bIgnoreCollision ? 1 : 0
	);
	// nPendingBotId is cleared inside InitializeDefaultProps::Call

	if (Bot == nullptr) {
		TgPawn__InitializeDefaultProps::nPendingBotId = 0;
		return nullptr;
	}

	ATgRepInfo_Player* BotRepInfo = reinterpret_cast<ATgRepInfo_Player*>(AIController->PlayerReplicationInfo);

	m_spawnedBotIds[(int)Bot] = nBotId;

	if (pOwnerPawn != nullptr) {
		AIController->SetOwner(pOwnerPawn);
	} else {
		AIController->SetOwner(WorldInfo);
	}

	Bot->r_bIsBot = 1;
	AIController->Pawn = Bot;
	Bot->Controller = AIController;
	Bot->PlayerReplicationInfo = AIController->PlayerReplicationInfo;
	Bot->Role = 3;
	Bot->PlayerReplicationInfo->Role = 3;
	AIController->Role = 3;

	Bot->r_EffectManager->r_Owner = Bot;
	Bot->r_EffectManager->SetOwner(Bot);
	Bot->r_EffectManager->Base = Bot;
	Bot->r_EffectManager->Role = 3;

	BotRepInfo->bBot = 1;
	BotRepInfo->r_PawnOwner = Bot;
	BotRepInfo->r_ApproxLocation = Bot->Location;

	if (pOwnerPawn != nullptr) {
		BotRepInfo->SetOwner(pOwnerPawn);
	} else {
		BotRepInfo->SetOwner(WorldInfo);
	}

	if (pOwnerPawn != nullptr) {
		Bot->SetOwner(pOwnerPawn);
	} else {
		Bot->SetOwner(BotRepInfo);
	}

	if (pOwnerPawn != nullptr) {
		Bot->r_Owner = pOwnerPawn;
		pOwnerPawn->r_Pet = Bot;
		AIController->m_pOwner = pOwnerPawn;
	}

	AIController->Possess(Bot, 0, 1);

	Bot->ApplyPawnSetup();
	Bot->WaitForInventoryThenDoPostPawnSetup();
	Bot->InvManager->Instigator = Bot;

	if (!CAmBot__LoadBotMarshal::m_BotPointers[nBotId]) {
		Logger::Log("debug", "Bot with id %d not found\n", nBotId);
		return nullptr;
	}

	void* BotConfig = (void*)CAmBot__LoadBotMarshal::m_BotPointers[nBotId];

	// offsets pulled from 0x1094c730
	Bot->r_nPhysicalType    = *(int*)((char*)BotConfig + 0x64); // PHYSICAL_TYPE_VALUE_ID
	Bot->r_nBodyMeshAsmId   = *(int*)((char*)BotConfig + 0x54); // BODY_ASM_ID
	Bot->s_nCharacterId     = *(int*)((char*)BotConfig + 0x5C); // BOT_TYPE_VALUE_ID
	Bot->r_nHealthMaximum   = *(int*)((char*)BotConfig + 0x74);
	Bot->Health             = *(int*)((char*)BotConfig + 0x74);
	Bot->HealthMax          = *(int*)((char*)BotConfig + 0x74);

	Bot->r_bIsStealthed = 0;

	BotRepInfo->r_nHealthCurrent  = *(int*)((char*)BotConfig + 0x74);
	BotRepInfo->r_nHealthMaximum  = *(int*)((char*)BotConfig + 0x74);
	BotRepInfo->r_nCharacterId    = *(int*)((char*)BotConfig + 0x5C);

	if (pFactory == nullptr) {
		ActorCache::CacheMapActors();
		pFactory = ActorCache::BotFactory;
		if (pFactory) {
			Logger::Log("debug", "TgBotFactory fallback found: %s\n", pFactory->GetFullName());
		}
	} else {
		Logger::Log("debug", "TgBotFactory passed: %s\n", pFactory->GetFullName());
	}

	TgAIController__InitBehavior::CallOriginal(AIController, edx, BotConfig, pFactory);

	BotRepInfo->bNetDirty = 1;
	BotRepInfo->bSkipActorPropertyReplication = 0;
	BotRepInfo->bOnlyDirtyReplication = 0;
	BotRepInfo->bNetInitial = 1;

	Bot->r_EffectManager->RemoteRole = 1;
	Bot->r_EffectManager->bNetInitial = 1;
	Bot->r_EffectManager->bNetDirty = 1;

	Bot->bNetInitial = 1;
	Bot->bNetDirty = 1;
	Bot->bReplicateMovement = 1;
	Bot->RemoteRole = 1;
	Bot->bSkipActorPropertyReplication = 0;

	GiveDevicesFromBotConfig(Bot, BotRepInfo, nBotId);

	return Bot;
}

