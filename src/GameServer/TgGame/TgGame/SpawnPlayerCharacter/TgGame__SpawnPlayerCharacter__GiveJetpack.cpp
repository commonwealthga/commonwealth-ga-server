#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/CreateDevice/AssemblyDatManager__CreateDevice.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"

ATgDevice* TgGame__SpawnPlayerCharacter::GiveJetpack(ATgPawn_Character *Pawn, ATgRepInfo_Player* PlayerReplicationInfo, int nInventoryId) {
	LogCallBegin("GiveJetpack");

	UTgInventoryObject_Device* InventoryObject = (UTgInventoryObject_Device*)TgInventoryObject_Device__ConstructInventoryObject::CallOriginal(
		ClassPreloader::GetTgInventoryObjectDeviceClass(),
		-1, 0, 0, 0, 0, 0, 0, nullptr);
	// add RF_RootSet flag 0x4000
	InventoryObject->ObjectFlags.A |= 0x4000;

	FInventoryData InventoryData;
	InventoryData.bBoundFlag = 0;
	InventoryData.bEquippedOnOtherChar = 0;
	InventoryData.nCreatedByCharacterId = 0;
	InventoryData.fAcquiredDatetime = 0.0f;
	InventoryData.m_nEquipSlotValueIdArray[0] = 806;
	InventoryData.nCraftedQualityValueId = 1162;
	InventoryData.nBlueprintId = 0;
	InventoryData.nDurability = 100;
	InventoryData.nInstanceCount = 1;
	InventoryData.nInvId = nInventoryId;
	InventoryData.nItemId = GA_G::DEVICE_ID_MEDIC_CJP;
	InventoryData.nLocationCode = 370;

	int equipPoint = GA_G::EQUIP_POINT_JETPACK_ID;
	

	InventoryObject->m_pAmItem.Dummy = CAmItem__LoadItemMarshal::m_ItemPointers[GA_G::DEVICE_ID_MEDIC_CJP];
	InventoryObject->m_InventoryData = InventoryData;
	InventoryObject->s_bPersistsInInventory = 1;
	InventoryObject->s_ReplicatedState = 0;
	InventoryObject->m_nDeviceInstanceId = nInventoryId;

	InventoryObject->m_InvManager = (ATgInventoryManager*)Pawn->InvManager;
	ATgDevice* Device = AssemblyDatManager__CreateDevice::CallOriginal(Globals::Get().GAssemblyDatManager, nullptr, InventoryObject->m_InventoryData.nItemId, Pawn);
	InventoryObject->s_Device = Device;

	if (Device != nullptr) {
		Device->s_InventoryObject = InventoryObject;
		Device->r_nDeviceInstanceId = nInventoryId;
		Device->m_bIsOffHand = 1;
		Device->m_bHandDevice = 0;
		Device->m_nDeviceType = 806;
		Device->r_nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
		Device->r_nQualityValueId = 1162;

		FEquipDeviceInfo EquipDeviceInfo;
		EquipDeviceInfo.nDeviceId = InventoryObject->m_InventoryData.nItemId;
		EquipDeviceInfo.nDeviceInstanceId = nInventoryId;
		EquipDeviceInfo.nQualityValueId = InventoryObject->m_InventoryData.nCraftedQualityValueId;

		Pawn->m_EquippedDevices[equipPoint] = Device;

		Pawn->r_EquipDeviceInfo[equipPoint] = EquipDeviceInfo;

		PlayerReplicationInfo->r_EquipDeviceInfo[equipPoint] = EquipDeviceInfo;

		Device->r_eEquippedAt = equipPoint;
		Device->r_nInventoryId = nInventoryId;
		Device->s_InventoryObject->m_InventoryData.nInvId = nInventoryId;

		// Logger::DumpMemory("newjetpack", Device, 0x2D8, 0);
		//
		// for (int i = 0; i < Device->m_FireMode.Num(); i++) {
		// 	UTgDeviceFire* FireMode = Device->m_FireMode.Data[i];
		// 	Logger::DumpMemory("newjetpackfire", FireMode, 0x164, 0);
		// }

		
		if ((char*)Pawn->InvManager + 0x1f0 == nullptr) {
			TMap__Allocate::CallOriginal((void*)((char*)Pawn->InvManager + 0x1f0));
		}
		if ((char*)Pawn->InvManager + 0x22c == nullptr) {
			TMap__Allocate::CallOriginal((void*)((char*)Pawn->InvManager + 0x22c));
		}
		TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)Pawn->InvManager + 0x1f0), nullptr, Device->s_InventoryObject->m_InventoryData.nInvId, Device->s_InventoryObject);
		TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)Pawn->InvManager + 0x22c), nullptr, Device->s_InventoryObject->m_InventoryData.nInvId, Device->s_InventoryObject);

		Device->SetOwner(Pawn);
		Device->Owner = Pawn;
		Device->Base = Pawn;
		Device->Instigator = Pawn;
		Device->Role = 3;
		Device->RemoteRole = 1;
		Device->bNetInitial = 1;
		Device->bNetDirty = 1;
		Device->bReplicateMovement = 1;
		Device->bSkipActorPropertyReplication = 0;
		Device->bOnlyDirtyReplication = 0;
		Device->bAlwaysRelevant = 1;
	}

	ATgDevice* CheckDevice = Pawn->GetDeviceByEqPoint(5);
	Logger::Log(GetLogChannel(), "GetDeviceByEqPoint(5) -> 0x%p\n", CheckDevice);
	Logger::Log(GetLogChannel(), "IsOffhandJetpack() -> %d\n", CheckDevice->IsOffhandJetpack());
	UTgDeviceFire* FireMode = CheckDevice->GetCurrentFire();
	Logger::Log(GetLogChannel(), "FireMode -> 0x%p\n", FireMode);
	Logger::Log(GetLogChannel(), "CanDeviceStartFiringNow() -> %d\n", CheckDevice->eventCanDeviceStartFiringNow(Device->CurrentFireMode, 1));
	Logger::Log(GetLogChannel(), "CanDeviceFireNow() -> %d\n", CheckDevice->eventCanDeviceFireNow(Device->CurrentFireMode, 1));
	Logger::Log(GetLogChannel(), "Pawn->IsNonCombat() -> %d\n", Pawn->IsNonCombat());
	Logger::Log(GetLogChannel(), "IsNonCombatJetpack() -> %d\n", CheckDevice->IsNonCombatJetpack());
	Logger::Log(GetLogChannel(), "IsGameTypeDisabled() -> %d\n", CheckDevice->IsGameTypeDisabled());

	LogCallEnd();

	return Device;
}

