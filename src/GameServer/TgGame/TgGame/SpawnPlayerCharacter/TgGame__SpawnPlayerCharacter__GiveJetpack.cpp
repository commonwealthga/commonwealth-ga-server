#include "src/GameServer/TgGame/TgGame/SpawnPlayerCharacter/TgGame__SpawnPlayerCharacter.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/CreateDevice/AssemblyDatManager__CreateDevice.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"

ATgDevice* TgGame__SpawnPlayerCharacter::GiveJetpack(ATgPawn_Character *Pawn, ATgRepInfo_Player* PlayerReplicationInfo, ATgPlayerController* PlayerController, int nInventoryId) {
	LogCallBegin("GiveJetpack");

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
	// 0xC9 = 201 = jetpack slot value ID, maps to equip point 5 via FUN_109a1320
	InventoryData.m_nEquipSlotValueIdArray[0] = 201;
	InventoryData.m_nEquipSlotValueIdArray[1] = 201;
	InventoryData.m_nEquipSlotValueIdArray[2] = 201;
	InventoryData.m_nEquipSlotValueIdArray[3] = 201;
	InventoryData.m_nEquipSlotValueIdArray[4] = 201;
	InventoryData.m_nEquipSlotValueIdArray[5] = 201;
	InventoryData.nCraftedQualityValueId = 1162;
	InventoryData.nBlueprintId = 0;
	InventoryData.nDurability = 100;
	InventoryData.nInstanceCount = 1;
	InventoryData.nInvId = nInventoryId;
	InventoryData.nItemId = GA_G::DEVICE_ID_MEDIC_CJP;
	InventoryData.nLocationCode = 369;

	int equipPoint = GA_G::EQUIP_POINT_JETPACK_ID;

	InventoryObject->m_pAmItem.Dummy = CAmItem__LoadItemMarshal::m_ItemPointers[GA_G::DEVICE_ID_MEDIC_CJP];
	InventoryObject->m_InventoryData = InventoryData;
	InventoryObject->s_bPersistsInInventory = 0;
	InventoryObject->s_ReplicatedState = 1;
	InventoryObject->m_nDeviceInstanceId = nInventoryId;

	InventoryObject->m_InvManager = (ATgInventoryManager*)Pawn->InvManager;

	// Add to m_InventoryMap only (not s_ReplicateMap — normal flow only uses 0x1F0)
	TgInventoryManager__PrepopulateInventoryId::CallOriginal((void*)((char*)Pawn->InvManager + 0x1f0), nullptr, nInventoryId, InventoryObject);
	// Keep r_ItemCount in sync with map entry count
	((ATgInventoryManager*)Pawn->InvManager)->r_ItemCount++;

	ATgDevice* Device = Pawn->CreateEquipDevice(nInventoryId, GA_G::DEVICE_ID_MEDIC_CJP, equipPoint);
	InventoryObject->s_Device = Device;

	if (Device != nullptr) {
		Device->s_InventoryObject = InventoryObject;
		Device->r_nDeviceInstanceId = nInventoryId;  // non-zero so UpdateClientDevices detects the change
		Device->m_bIsOffHand = 1;
		Device->m_bHandDevice = 0;
		Device->m_nDeviceType = 806;
		Device->r_nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
		Device->r_nQualityValueId = 1162;

		FEquipDeviceInfo EquipDeviceInfo;
		EquipDeviceInfo.nDeviceId = GA_G::DEVICE_ID_MEDIC_CJP;
		EquipDeviceInfo.nDeviceInstanceId = nInventoryId;
		EquipDeviceInfo.nQualityValueId = InventoryObject->m_InventoryData.nCraftedQualityValueId;

		Pawn->m_EquippedDevices[equipPoint] = Device;

		// Do NOT set Pawn->r_EquipDeviceInfo here — let UpdateClientDevices detect the mismatch.
		// (See GiveDeviceById for the full explanation.)
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

	ATgDevice* CheckDevice = Pawn->GetDeviceByEqPoint(5);
	Logger::Log(GetLogChannel(), "GetDeviceByEqPoint(5) -> 0x%p\n", CheckDevice);
	Logger::Log(GetLogChannel(), "IsOffhandJetpack() -> %d\n", CheckDevice->IsOffhandJetpack());
	UTgDeviceFire* FireMode = CheckDevice->GetCurrentFire();
	Logger::Log(GetLogChannel(), "FireMode -> 0x%p\n", FireMode);
	Logger::Log(GetLogChannel(), "CanDeviceStartFiringNow() -> %d\n", CheckDevice->eventCanDeviceStartFiringNow(CheckDevice->CurrentFireMode, 1));
	Logger::Log(GetLogChannel(), "CanDeviceFireNow() -> %d\n", CheckDevice->eventCanDeviceFireNow(CheckDevice->CurrentFireMode, 1));
	Logger::Log(GetLogChannel(), "Pawn->IsNonCombat() -> %d\n", Pawn->IsNonCombat());
	Logger::Log(GetLogChannel(), "IsNonCombatJetpack() -> %d\n", CheckDevice->IsNonCombatJetpack());
	Logger::Log(GetLogChannel(), "IsGameTypeDisabled() -> %d\n", CheckDevice->IsGameTypeDisabled());
	Logger::Log(GetLogChannel(), "Controller->bCinematicMode -> %d\n", PlayerController->bCinematicMode);
	Logger::Log(GetLogChannel(), "m_nDeviceType -> %d\n", CheckDevice->m_nDeviceType);
	Logger::Log(GetLogChannel(), "Pawn->c_bDisableAction -> %d\n", Pawn->c_bDisableAction);
	Logger::Log(GetLogChannel(), "FireMode->GetAttackRate() -> %f\n", CheckDevice->m_FireMode.Data[CheckDevice->CurrentFireMode]->GetAttackRate());
	Logger::Log(GetLogChannel(), "FireMode->m_bRestrictInCombat -> %d\n", CheckDevice->m_FireMode.Data[CheckDevice->CurrentFireMode]->m_bRestrictInCombat);
	Logger::Log(GetLogChannel(), "Pawn->InCombat() -> %d\n", Pawn->InCombat());
	Logger::Log(GetLogChannel(), "FireMode->CheckSimutainousFiring() -> 0x%p\n", CheckDevice->m_FireMode.Data[CheckDevice->CurrentFireMode]->CheckSimutainousFiring());
	Logger::Log(GetLogChannel(), "CanUseDeviceInThisPhysicsState() -> %d\n", CheckDevice->CanUseDeviceInThisPhysicsState(CheckDevice->CurrentFireMode));

	// Diagnose r_FlightAcceleration property chain
	Logger::Log(GetLogChannel(), "r_FlightAcceleration (0xF84) = %f\n", *(float*)((char*)Pawn + 0xF84));
	Logger::Log(GetLogChannel(), "Pawn->Controller (0x1D8) = 0x%p\n", *(void**)((char*)Pawn + 0x1D8));
	// Check if property 59 (TGPID_FLIGHT_ACCELERATION) is in s_Properties via vtable+0x4F0 (GetPropertyByID)
	typedef void* (__thiscall *GetPropertyByIDFn)(ATgPawn*, int);
	void** vtable = *(void***)Pawn;
	void* prop59  = ((GetPropertyByIDFn)vtable[0x4F0 / 4])(Pawn, 59);   // TGPID_FLIGHT_ACCELERATION
	void* prop243 = ((GetPropertyByIDFn)vtable[0x4F0 / 4])(Pawn, 243);  // TGPID_CURRENT_POWER_POOL (known to work)
	Logger::Log(GetLogChannel(), "GetPropertyByID(59/FLIGHT_ACCEL) = 0x%p\n", prop59);
	Logger::Log(GetLogChannel(), "GetPropertyByID(243/POWER_POOL)  = 0x%p  (sanity check)\n", prop243);

	LogCallEnd();

	return Device;
}

