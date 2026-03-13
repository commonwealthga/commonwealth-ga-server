#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/CreateDevice/AssemblyDatManager__CreateDevice.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"

ATgDevice* TgGame__SpawnPlayerCharacter::GiveAgonizer(ATgPawn_Character *Pawn, ATgRepInfo_Player* PlayerReplicationInfo, ATgPlayerController* PlayerController, int nInventoryId) {
	LogCallBegin("GiveAgonizer");

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
	InventoryData.m_nEquipSlotValueIdArray[0] = 198;
	InventoryData.m_nEquipSlotValueIdArray[1] = 198;
	InventoryData.m_nEquipSlotValueIdArray[2] = 198;
	InventoryData.m_nEquipSlotValueIdArray[3] = 198;
	InventoryData.m_nEquipSlotValueIdArray[4] = 198;
	InventoryData.m_nEquipSlotValueIdArray[5] = 198;
	InventoryData.nCraftedQualityValueId = 1162;
	InventoryData.nBlueprintId = 0;
	InventoryData.nDurability = 100;
	InventoryData.nInstanceCount = 1;
	InventoryData.nInvId = nInventoryId;
	InventoryData.nItemId = GA_G::DEVICE_ID_AGONIZER;
	InventoryData.nLocationCode = 369;

	int equipPoint = 2; // ranged weapon slot (slot value ID 198 → equip point 2)

	InventoryObject->m_pAmItem.Dummy = CAmItem__LoadItemMarshal::m_ItemPointers[GA_G::DEVICE_ID_AGONIZER];
	InventoryObject->m_InventoryData = InventoryData;
	InventoryObject->s_bPersistsInInventory = 0;
	InventoryObject->s_ReplicatedState = 1;
	InventoryObject->m_nDeviceInstanceId = nInventoryId;

	InventoryObject->m_InvManager = (ATgInventoryManager*)Pawn->InvManager;

	// Add to m_InventoryMap only (not s_ReplicateMap — normal flow only uses 0x1F0)
	TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)Pawn->InvManager + 0x1f0), nullptr, nInventoryId, InventoryObject);
	// Keep r_ItemCount in sync with map entry count
	((ATgInventoryManager*)Pawn->InvManager)->r_ItemCount++;

	ATgDevice* Device = Pawn->CreateEquipDevice(nInventoryId, GA_G::DEVICE_ID_AGONIZER, equipPoint);
	InventoryObject->s_Device = Device;

	if (Device != nullptr) {
		Device->s_InventoryObject = InventoryObject;
		Device->r_nDeviceInstanceId = nInventoryId;  // non-zero so UpdateClientDevices detects the change
		Device->m_bIsOffHand = 0;
		Device->m_bHandDevice = 1;
		Device->m_nDeviceType = 388;
		Device->r_nDeviceId = GA_G::DEVICE_ID_AGONIZER;
		Device->r_nQualityValueId = 1162;

		FEquipDeviceInfo EquipDeviceInfo;
		EquipDeviceInfo.nDeviceId = GA_G::DEVICE_ID_AGONIZER;
		EquipDeviceInfo.nDeviceInstanceId = nInventoryId;
		EquipDeviceInfo.nQualityValueId = InventoryObject->m_InventoryData.nCraftedQualityValueId;

		Pawn->m_EquippedDevices[equipPoint] = Device;

		Pawn->r_EquipDeviceInfo[equipPoint] = EquipDeviceInfo;

		PlayerReplicationInfo->r_EquipDeviceInfo[equipPoint] = EquipDeviceInfo;

		Device->r_eEquippedAt = equipPoint;
		Pawn->r_eDesiredInHand = equipPoint;
		Device->r_nInventoryId = nInventoryId;
		Device->s_InventoryObject->m_InventoryData.nInvId = nInventoryId;

		// Logger::DumpMemory("newjetpack", Device, 0x2D8, 0);
		//
		// for (int i = 0; i < Device->m_FireMode.Num(); i++) {
		// 	UTgDeviceFire* FireMode = Device->m_FireMode.Data[i];
		// 	Logger::DumpMemory("newjetpackfire", FireMode, 0x164, 0);
		// }


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

		// UpdateClientDevices now detects r_nDeviceInstanceId mismatch (nInventoryId vs 0),
		// updates r_EquipDeviceInfo, and fires the equip handler that drives client-side setup.
		Pawn->UpdateClientDevices(0, 0);

		Pawn->bNetDirty = 1;
		Pawn->bForceNetUpdate = 1;
	}

	LogCallEnd();

	return Device;
}

