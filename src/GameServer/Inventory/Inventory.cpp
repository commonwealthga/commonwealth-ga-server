// Logger channel: "inventory"

#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/EquipSlot.hpp"
#include "src/GameServer/Constants/Quality.hpp"
#include "src/GameServer/Constants/DeviceIds.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/Utils/Logger/Logger.hpp"

// ---------------------------------------------------------------------------
// Static member initialization
// ---------------------------------------------------------------------------
int Inventory::s_nextInventoryId = 10000;
std::map<ATgPawn*, std::vector<EquippedEntry>> Inventory::s_equipped;
std::vector<EquippedEntry> Inventory::s_empty;

// ---------------------------------------------------------------------------
// Private helpers — auto-detect device metadata from equip slot
// ---------------------------------------------------------------------------

int Inventory::GetDeviceType(int slot) {
	switch (slot) {
		case GA::EquipSlot::Melee:      return GA_G::TGDT_MELEE;            // 389
		case GA::EquipSlot::Ranged:     return GA_G::TGDT_RANGED;           // 388
		case GA::EquipSlot::Specialty:  return GA_G::TGDT_SPECIALTY;        // 981
		case GA::EquipSlot::Specialty2: return GA_G::TGDT_SPECIALTY;        // 981
		case GA::EquipSlot::Jetpack:    return GA_G::TGDT_TRAVEL;           // 806
		case GA::EquipSlot::Offhand1:   return GA_G::TGDT_OFF_HAND;        // 390
		case GA::EquipSlot::Offhand2:   return GA_G::TGDT_OFF_HAND;        // 390
		case GA::EquipSlot::Offhand3:   return GA_G::TGDT_OFF_HAND;        // 390
		case GA::EquipSlot::Morale:     return GA_G::TGDT_MORALE;           // 476
		case GA::EquipSlot::Rest:       return GA_G::TGDT_BASE_HUMAN_ATTRIB; // 392
		case GA::EquipSlot::Beacon:     return GA_G::TGDT_PLAYER_SENSOR;    // 393
		case GA::EquipSlot::Consumable: return GA_G::TGDT_OFF_HAND;        // 390
		default:                        return 0;
	}
}

bool Inventory::IsOffHand(int slot) {
	// Jetpack and Offhand1/2/3 are offhand slots
	return slot == GA::EquipSlot::Jetpack
		|| slot == GA::EquipSlot::Offhand1
		|| slot == GA::EquipSlot::Offhand2
		|| slot == GA::EquipSlot::Offhand3;
}

bool Inventory::IsHandDevice(int slot) {
	// Melee, Ranged, and Specialty are hand device slots
	return slot == GA::EquipSlot::Melee
		|| slot == GA::EquipSlot::Ranged
		|| slot == GA::EquipSlot::Specialty;
}

// ---------------------------------------------------------------------------
// Inventory::Equip — create and equip a device on a pawn
// ---------------------------------------------------------------------------
ATgDevice* Inventory::Equip(ATgPawn* Pawn, int deviceId, int slot, int quality) {
	// --- Null checks ---
	if (Pawn == nullptr) {
		Logger::Log("inventory", "Equip: ERROR — Pawn is null\n");
		return nullptr;
	}
	if (Pawn->InvManager == nullptr) {
		Logger::Log("inventory", "Equip: ERROR — Pawn 0x%p has null InvManager\n", Pawn);
		return nullptr;
	}

	// --- Assign unique inventory ID ---
	int invId = ++s_nextInventoryId;

	// --- Resolve slot value ID ---
	int slotValueId = GA::SlotValueId(slot);
	if (slotValueId == 0) {
		Logger::Log("inventory", "Equip: ERROR — invalid slot %d (no slot value ID mapping)\n", slot);
		return nullptr;
	}

	// --- Quality: 0 means "no quality" per API-06; pass through as-is ---
	int qualityValueId = quality;

	// --- Create inventory object ---
	UTgInventoryObject_Device* InvObj = (UTgInventoryObject_Device*)
		TgInventoryObject_Device__ConstructInventoryObject::CallOriginal(
			ClassPreloader::GetTgInventoryObjectDeviceClass(),
			-1, 0, 0, 0, 0, 0, 0, nullptr);
	InvObj->ObjectFlags.A |= 0x4000;  // RF_RootSet

	// --- Fill FInventoryData ---
	FInventoryData data;
	data.bBoundFlag = 1;
	data.bEquippedOnOtherChar = 0;
	data.nCreatedByCharacterId = 998;
	data.fAcquiredDatetime = 1700000000.0f;
	for (int i = 0; i < 6; i++) data.m_nEquipSlotValueIdArray[i] = slotValueId;
	data.nCraftedQualityValueId = qualityValueId;
	data.nBlueprintId = 0;
	data.nDurability = 100;
	data.nInstanceCount = 1;
	data.nInvId = invId;
	data.nItemId = deviceId;
	data.nLocationCode = 369;

	// --- Wire inventory object ---
	InvObj->m_pAmItem.Dummy = CAmItem__LoadItemMarshal::m_ItemPointers[deviceId];
	InvObj->m_InventoryData = data;
	InvObj->s_bPersistsInInventory = 0;
	InvObj->s_ReplicatedState = 1;
	InvObj->m_nDeviceInstanceId = invId;
	InvObj->m_InvManager = (ATgInventoryManager*)Pawn->InvManager;

	// --- Add to inventory map + sync item count ---
	TgInventoryManager__PrepopulateInventoryId::CallOriginal(
		(void*)((char*)Pawn->InvManager + 0x1f0), nullptr, invId, InvObj);
	((ATgInventoryManager*)Pawn->InvManager)->r_ItemCount++;

	// --- Create device via native function ---
	ATgDevice* Device = Pawn->CreateEquipDevice(invId, deviceId, slot);
	InvObj->s_Device = Device;

	if (Device == nullptr) {
		Logger::Log("inventory", "Equip: WARNING — CreateEquipDevice returned null for pawn=0x%p device=%d slot=%d\n",
			Pawn, deviceId, slot);
		return nullptr;
	}

	// --- Set all device fields ---
	Device->s_InventoryObject = InvObj;
	Device->r_nDeviceInstanceId = invId;  // non-zero so UpdateClientDevices detects the change
	Device->m_bIsOffHand = IsOffHand(slot) ? 1 : 0;
	Device->m_bHandDevice = IsHandDevice(slot) ? 1 : 0;
	Device->m_nDeviceType = GetDeviceType(slot);
	Device->r_nDeviceId = deviceId;
	Device->r_nQualityValueId = qualityValueId;

	FEquipDeviceInfo eqInfo;
	eqInfo.nDeviceId = deviceId;
	eqInfo.nDeviceInstanceId = invId;
	eqInfo.nQualityValueId = qualityValueId;

	Pawn->m_EquippedDevices[slot] = Device;

	// Do NOT set Pawn->r_EquipDeviceInfo[slot] here — let UpdateClientDevices
	// detect the mismatch (device has invId, r_EquipDeviceInfo still has 0).
	// This triggers the equip handler + ProcessEvent for client-side setup.

	// Set PRI r_EquipDeviceInfo
	ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (PRI != nullptr) {
		PRI->r_EquipDeviceInfo[slot] = eqInfo;
	}

	Device->r_eEquippedAt = slot;
	if (IsHandDevice(slot)) Pawn->r_eDesiredInHand = slot;
	Device->r_nInventoryId = invId;
	Device->s_InventoryObject->m_InventoryData.nInvId = invId;

	// --- REST device auto-config (deviceId == 864) ---
	if (deviceId == GA::DeviceId::RestDevice) {
		*(uint32_t*)((char*)Device + 0x22C) |= 0x00020000;  // m_bIsRestDevice
		*(ATgDevice**)((char*)Pawn + 0x0904) = Device;        // m_RestDevice
		*(int*)((char*)Pawn + 0x1524) = slot;                  // r_nRestDeviceSlot
	}

	// --- Morale device auto-config ---
	if (GetDeviceType(slot) == GA_G::TGDT_MORALE) {
		*(int*)((char*)Pawn + 0x1520) = slot;  // r_nMoraleDeviceSlot
	}

	// --- Replication flags ---
	Device->Instigator = (APawn*)Pawn;
	Device->Base = Pawn;
	Device->Role = 3;
	Device->RemoteRole = 1;
	Device->bNetInitial = 1;
	Device->bNetDirty = 1;
	Device->bReplicateMovement = 1;
	Device->bSkipActorPropertyReplication = 0;
	Device->bOnlyDirtyReplication = 0;

	// --- Track in per-pawn map ---
	s_equipped[Pawn].push_back({deviceId, slot, qualityValueId, invId});

	Logger::Log("inventory", "Equip: pawn=0x%p device=%d slot=%d quality=%d invId=%d -> device=0x%p\n",
		Pawn, deviceId, slot, qualityValueId, invId, Device);
	return Device;
}

// ---------------------------------------------------------------------------
// Inventory::Finalize — trigger replication after all equips
// ---------------------------------------------------------------------------
void Inventory::Finalize(ATgPawn* Pawn) {
	if (Pawn == nullptr) return;

	Pawn->UpdateClientDevices(0, 0);
	Pawn->bNetDirty = 1;
	Pawn->bForceNetUpdate = 1;

	ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
	if (PRI != nullptr) {
		PRI->bNetDirty = 1;
		PRI->bForceNetUpdate = 1;
	}

	Logger::Log("inventory", "Finalize: pawn=0x%p, %d devices tracked\n",
		Pawn, (int)s_equipped[Pawn].size());
}

// ---------------------------------------------------------------------------
// Inventory::GetEquipped — query tracked devices for a pawn
// ---------------------------------------------------------------------------
const std::vector<EquippedEntry>& Inventory::GetEquipped(ATgPawn* Pawn) {
	auto it = s_equipped.find(Pawn);
	if (it != s_equipped.end()) return it->second;
	return s_empty;
}

// ---------------------------------------------------------------------------
// Inventory::ClearTracking — remove pawn from tracking map
// ---------------------------------------------------------------------------
void Inventory::ClearTracking(ATgPawn* Pawn) {
	s_equipped.erase(Pawn);
}
