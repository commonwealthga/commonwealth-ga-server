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
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

// ---------------------------------------------------------------------------
// Static member initialization
// ---------------------------------------------------------------------------
int Inventory::s_nextInventoryId = 10000;
std::map<ATgPawn*, std::vector<EquippedEntry>> Inventory::s_equipped;
std::map<int, std::vector<EquippedEntry>*> Inventory::s_equippedByPawnId;
std::vector<EquippedEntry> Inventory::s_empty;

// ---------------------------------------------------------------------------
// Inventory::NextId — single source of truth for unique inventory IDs
// ---------------------------------------------------------------------------
int Inventory::NextId() {
	return ++s_nextInventoryId;
}

// ---------------------------------------------------------------------------
// Inventory::GetEffectGroupId — hardcoded effect group lookup by device ID
// Replace with DB lookup when device->effect_group relation is added to DB.
// ---------------------------------------------------------------------------
int Inventory::GetEffectGroupId(int deviceId) {
	switch (deviceId) {
		case 7032: return 26173;  // jetpack (Medic CJP)
		case 2991: return 16670;  // agonizer/ranged
		case 2531: return 16653;  // healing grenade/offhand
		case 5800: return 22334;  // melee/LifeStealer
		case 2906: return 9071;   // specialty (used in medic bot config)
		default:   return 0;      // REST, morale, unknown
	}
}

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
// Inventory::ApplyDeviceEquipEffects — DB-driven reimplementation of the
// stripped asm.dat equip-effect path (UC ApplyEquipEffects → m_EquipEffect →
// ProcessEffect). Applies every permanent (lifetime_sec=0) effect attached to
// the device's equip-effect groups directly to the pawn's properties.
//
// Calc-method semantics follow TgEffect.uc:448 (apply, !bRemove):
//   67 ADD             newRaw = curRaw + base
//   70 SUBTRACT        newRaw = curRaw - base
//   68 PERC_INCREASE   newRaw = curRaw + (propBase * base)
//   69 PERC_DECREASE   newRaw = curRaw - (propBase * base)
//   119 NA / other     no-op
//
// IMPORTANT — we mutate prop->m_fRaw DIRECTLY rather than calling
// Pawn->SetProperty(). The native SetProperty (0x109bf420) does its property
// lookup through vtable[0x4F0] which uses the TMap at pawn+0x400; that map
// is never populated for properties added by our InitializeProperty path
// (only the s_Properties array gets the entry). So SetProperty silently
// no-ops for protection / vision / etc. Same workaround SyncPawnHealth uses
// for HP. Direct write is correct here because every consumer reads
// prop->m_fRaw (CalcProtection in TgEffectGroup.uc:757 calls
// FClamp(m_fRaw, m_fMinimum, m_fMaximum), so out-of-range values are
// clamped at read time — no need to clamp at write).
//
// The DISTINCT is load-bearing: asm_data_set_effects and
// asm_data_set_effect_groups currently contain duplicate rows because the
// capture path runs twice during startup. Without it, +30 protection becomes
// +60.
// ---------------------------------------------------------------------------
void Inventory::ApplyDeviceEquipEffects(ATgPawn* Pawn, int deviceId) {
	if (Pawn == nullptr || Pawn->s_Properties.Data == nullptr) return;

	sqlite3* db = Database::GetConnection();
	if (db == nullptr) return;

	static const char* kSql =
		"SELECT DISTINCT e.prop_id, e.base_value, e.calc_method_value_id "
		"FROM asm_data_set_device_effect_groups deg "
		"JOIN asm_data_set_effect_groups eg ON eg.effect_group_id = deg.effect_group_id "
		"JOIN asm_data_set_effects e ON e.effect_group_id = deg.effect_group_id "
		"WHERE deg.device_id = ? AND eg.lifetime_sec = 0";

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("inventory", "ApplyDeviceEquipEffects: prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int(stmt, 1, deviceId);

	int applied = 0, skipped = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int   propId     = sqlite3_column_int   (stmt, 0);
		float baseValue  = (float)sqlite3_column_double(stmt, 1);
		int   calcMethod = sqlite3_column_int   (stmt, 2);

		UTgProperty* prop = nullptr;
		for (int i = 0; i < Pawn->s_Properties.Num(); ++i) {
			UTgProperty* p = Pawn->s_Properties.Data[i];
			if (p && p->m_nPropertyId == propId) { prop = p; break; }
		}
		if (prop == nullptr) {
			Logger::Log("inventory", "ApplyDeviceEquipEffects: pawn=0x%p device=%d propId=%d not in s_Properties — skipped\n",
				Pawn, deviceId, propId);
			++skipped;
			continue;
		}

		const float curRaw   = prop->m_fRaw;
		const float propBase = prop->m_fBase;
		float newRaw;
		switch (calcMethod) {
			case 67: newRaw = curRaw + baseValue;              break;
			case 70: newRaw = curRaw - baseValue;              break;
			case 68: newRaw = curRaw + (propBase * baseValue); break;
			case 69: newRaw = curRaw - (propBase * baseValue); break;
			default: continue;
		}
		prop->m_fRaw = newRaw;
		Logger::Log("inventory",
			"ApplyDeviceEquipEffects: pawn=0x%p device=%d propId=%d cm=%d base=%.2f  m_fRaw %.2f -> %.2f\n",
			Pawn, deviceId, propId, calcMethod, baseValue, curRaw, newRaw);
		++applied;
	}
	sqlite3_finalize(stmt);

	if (applied > 0 || skipped > 0) {
		Logger::Log("inventory", "ApplyDeviceEquipEffects: pawn=0x%p device=%d applied=%d skipped=%d\n",
			Pawn, deviceId, applied, skipped);
	}
}

// ---------------------------------------------------------------------------
// Inventory::Equip — create and equip a device on a pawn
// ---------------------------------------------------------------------------
ATgDevice* Inventory::Equip(ATgPawn* Pawn, int deviceId, int slot, int quality, int inventoryId) {
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
	int invId = (inventoryId > 0) ? inventoryId : NextId();

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
	for (int i = 0; i < 5; i++) data.m_nEquipSlotValueIdArray[i] = slotValueId;
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
		Device->m_bIsRestDevice = 1;
		Pawn->m_RestDevice = Device;
		Pawn->r_nRestDeviceSlot = slot;
	}

	// --- Morale device auto-config ---
	if (GetDeviceType(slot) == GA_G::TGDT_MORALE) {
		Pawn->r_nMoraleDeviceSlot = slot;
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

	// --- Apply permanent equip-effect groups (DB-driven) ---
	// Stand-in for the stripped asm.dat → device->m_EquipEffect path. Most
	// importantly, this is what gives every pawn the +30 physical protection
	// baseline via device 864 (HUMAN BASE ATTRIBUTES).
	ApplyDeviceEquipEffects(Pawn, deviceId);

	// --- Track in per-pawn map ---
	s_equipped[Pawn].push_back({deviceId, slot, qualityValueId, invId, GetEffectGroupId(deviceId)});

	// --- Update pawnId -> vector* lookup ---
	int pawnId = Pawn->r_nPawnId;
	s_equippedByPawnId[pawnId] = &s_equipped[Pawn];

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
// Inventory::GetEquippedByPawnId — query tracked devices for a pawn by ID
// ---------------------------------------------------------------------------
const std::vector<EquippedEntry>& Inventory::GetEquippedByPawnId(int pawnId) {
	auto it = s_equippedByPawnId.find(pawnId);
	if (it != s_equippedByPawnId.end() && it->second != nullptr) return *(it->second);
	return s_empty;
}

// ---------------------------------------------------------------------------
// Inventory::ClearTracking — remove pawn from tracking maps
// ---------------------------------------------------------------------------
void Inventory::ClearTracking(ATgPawn* Pawn) {
	// Remove from pawnId lookup first (pointer into s_equipped, must erase before s_equipped)
	auto* vec = &s_equipped[Pawn];
	for (auto it = s_equippedByPawnId.begin(); it != s_equippedByPawnId.end(); ) {
		if (it->second == vec) {
			it = s_equippedByPawnId.erase(it);
		} else {
			++it;
		}
	}
	s_equipped.erase(Pawn);
}
