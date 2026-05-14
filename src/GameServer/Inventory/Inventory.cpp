// Logger channel: "inventory"

#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/TgGame/TgPawn/ApplyBuff/TgPawn__ApplyBuff.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/DeviceCategorySkill.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Constants/EquipSlot.hpp"
#include "src/GameServer/Constants/Quality.hpp"
#include "src/GameServer/Constants/DeviceIds.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <algorithm>
#include <string>

// ---------------------------------------------------------------------------
// Static member initialization
// ---------------------------------------------------------------------------
// Start far above the player-device range. Player device inventory_ids are
// minted by PlayerSessionStore::ResyncCharacterDevicesFromLoadout as
// `10000 + equip_slot` (i.e. 10001..10025+), so bot devices minted from this
// counter MUST stay outside that window. The original starting value of
// 10000 produced direct collisions: bot device #1 got 10001 = player slot 1.
// Result was the client's `FUN_10a14a50` device-by-instance-id lookup
// returning whichever ATgDevice (player vs bot) had been registered first
// for that id — so slot bindings, c_DeviceForm pointers, and cooldown timers
// silently swapped between pawns during possession. Verified 2026-05-12.
int Inventory::s_nextInventoryId = 1000000;
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
// Inventory::ApplyRolledModEffects — register rolled-mod effects with the
// pawn's buff system (m_EffectBuffInfo TArray @ +0x100C). Each effect maps
// to one TgPawn::ApplyBuff call.
//
// Why the buff system rather than direct s_Properties writes:
//   The original TgDeviceFire::GetPropertyValueById (binary @ 0x10a27d40) is
//   already correct — when it reads a fire-mode param, it routes through
//   vtable[0x570] (ConvertPropToPropList) and vtable[0x56C] (the formula
//   function decompiled at 0x109cd4a0) to apply layered Item/Skill/Self/
//   Generic modifiers. ConvertPropToPropList specifically expands a request
//   for fire-mode prop 6 (Effect Radius) into {6, 352} so an entry registered
//   under the AOE Radius Modifier prop_id (352) is summed in. The single
//   missing piece was that TgPawn::ApplyBuff was a stripped no-op, so the
//   buff registry stayed empty. Reimplemented in TgPawn__ApplyBuff::Call.
//
// Source-type for rolled mods is BUFF_SOURCE_TYPE_ITEM (=1) → fItem* slots.
// ---------------------------------------------------------------------------
void Inventory::ApplyRolledModEffects(ATgPawn* Pawn, int deviceId, int deviceInstanceId, const std::vector<int>& effectGroupIds) {
	if (Pawn == nullptr) return;
	if (effectGroupIds.empty()) return;

	sqlite3* db = Database::GetConnection();
	if (db == nullptr) return;

	// Build the SQL IN-clause from the *unique* set of egids (SQLite would
	// collapse `IN (X,X,X)` to `IN (X)` anyway). Then iterate the *full* input
	// list — duplicates included — and emit one ApplyBuff per occurrence: each
	// mod kit slotted in the device is a separate stacking instance, e.g. 6×
	// AOE Radius +15% must produce IP=90, not IP=15.
	//
	// Earlier shape used `SELECT DISTINCT ...` and applied one effect per
	// distinct row, which silently collapsed N identical mods into a single
	// buff entry — log evidence: "applied=1 effect(s) from 6 group(s)" with
	// 320→368 instead of 320→608.
	std::vector<int> uniqueIds(effectGroupIds.begin(), effectGroupIds.end());
	std::sort(uniqueIds.begin(), uniqueIds.end());
	uniqueIds.erase(std::unique(uniqueIds.begin(), uniqueIds.end()), uniqueIds.end());

	// `asm_data_set_effect_groups` has duplicate rows per effect_group_id (the
	// asm.dat capture pulls each row twice for unrelated reasons), so a plain
	// JOIN doubled every effect — `applied=12 from 6 group(s)` instead of 6,
	// inflating the fItemPercent slot to 2× its intended value. Filter the
	// `lifetime_sec=0` constraint via a subquery instead so the JOIN can't
	// multiply rows.
	std::string sql =
		"SELECT e.effect_group_id, e.prop_id, e.base_value, e.calc_method_value_id "
		"FROM asm_data_set_effects e "
		"WHERE e.effect_group_id IN (SELECT effect_group_id FROM asm_data_set_effect_groups WHERE lifetime_sec = 0) "
		"  AND e.effect_group_id IN (";
	for (size_t i = 0; i < uniqueIds.size(); ++i) {
		if (i) sql += ',';
		sql += std::to_string(uniqueIds[i]);
	}
	sql += ')';

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("inventory", "ApplyRolledModEffects: prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}

	struct EffectRow { int propId; float baseValue; int calcMethod; };
	std::map<int, std::vector<EffectRow>> effectsByEgid;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		int   egid       = sqlite3_column_int   (stmt, 0);
		int   propId     = sqlite3_column_int   (stmt, 1);
		float baseValue  = (float)sqlite3_column_double(stmt, 2);
		int   calcMethod = sqlite3_column_int   (stmt, 3);

		// Calc methods 67/68/69/70 are the only ones our reimplemented
		// ApplyBuff handles. Skip anything else (effects targeting
		// non-modifier props, conditional effects, etc.).
		if (calcMethod < 67 || calcMethod > 70) continue;

		effectsByEgid[egid].push_back({ propId, baseValue, calcMethod });
	}
	sqlite3_finalize(stmt);

	int applied = 0;
	for (int egid : effectGroupIds) {
		auto it = effectsByEgid.find(egid);
		if (it == effectsByEgid.end()) continue;
		for (const EffectRow& r : it->second) {
			// Tag the buff entry with the equipping device's instance id so
			// the registry naturally per-device-scopes mods. Without this,
			// every equipped device's mods land in one shared (propId, 0,0,0)
			// entry — Output Mod (prop 385, on every device) collapses 9
			// entries into one with fItemPercent ≈ 700.
			//
			// FUN_109cd890 search-mode match rule:
			//   stored devInst > 0 → match only when query devInst equals it
			//   stored devInst == 0 → wildcard (matches any query)
			// Skills register with devInst=0 (wildcard) so they continue to
			// apply across all this pawn's devices. Cross-pawn aura buffs
			// (Techro station → friendly's accuracy) carry whatever devInst
			// their effect group's m_nReqDeviceInstanceId says — typically 0.
			FBuffHeader header{};
			header.nPropId          = r.propId;
			header.nReqCategoryCode = 0;
			header.nReqSkillId      = 0;
			header.nReqDeviceInstId = deviceInstanceId;

			// Output Mod (prop 385) is the universal "secondary output" multiplier
			// in every ConvertPropToPropList expansion that mentions it (51→{330,
			// 385} for heal, 51→{65, 385, …} for damage, 318→{357, 385}, 339→
			// {339, 385}). The PRIMARY modifier in each list (330/65/357/339)
			// belongs in the ITEM layer of BuffFormula — it sums with other
			// rolled mods of the same prop additively, then multiplies as one
			// factor. Output Mod must compose ON TOP of that, in a separate
			// multiplicative layer; otherwise +8/+9% Heal (IP=17) and +70%
			// Output Mod (IP=70) collapse into a single IP=87 bucket and the
			// cross-term is lost (Medical Station heal: 446/tick instead of
			// 473/tick — verified empirically 2026-05-09).
			//
			// Routing 385 to BUFF_SOURCE_TYPE_OTHER (=4) lands it in
			// fPercentModifier, which BuffFormula consumes as the
			// SELF+GENERIC layer `(1 + (ΣLP+ΣGP)/100)`. This composes with
			// the ITEM and SKILL layers as separate factors, matching the
			// original game's 1.17 × 1.55 × 1.70 ≈ 3.07× multiplier shape.
			//
			// ApplyPlayerModsToDeployable's layered formula reads the same
			// FBuffInfo entry through the same 8-float lens, so changing the
			// slot from IP to GP is a no-op for deploy-time s_Properties
			// scaling: when Output Mod is the only entry on prop 385, the
			// formula collapses to base × (1 + GP/100) either way.
			const unsigned char srcType =
				(r.propId == 385) ? /*OTHER*/4 : /*ITEM*/1;

			TgPawn__ApplyBuff::Call(Pawn, /*edx=*/nullptr, header, r.calcMethod, r.baseValue,
			                        /*bRemove=*/0, srcType);

			Logger::Log("inventory",
				"ApplyRolledModEffects: pawn=0x%p device=%d devInst=%d egid=%d propId=%d cm=%d base=%.2f -> ApplyBuff\n",
				Pawn, deviceId, deviceInstanceId, egid, r.propId, r.calcMethod, r.baseValue);
			++applied;
		}
	}

	if (applied > 0) {
		Logger::Log("inventory", "ApplyRolledModEffects: pawn=0x%p device=%d applied=%d effect(s) from %zu group(s)\n",
			Pawn, deviceId, applied, effectGroupIds.size());
	}
}

// ---------------------------------------------------------------------------
// Inventory::Equip — create and equip a device on a pawn
// ---------------------------------------------------------------------------
ATgDevice* Inventory::Equip(ATgPawn* Pawn, int deviceId, int slot, int quality, int inventoryId,
                            const std::vector<int>& mods) {
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

	// `m_nSkillId` (offset 0x2B0) — the device's classifying skill_id from
	// asm_data_set_items.skill_id. The binary's TgDeviceFire::GetPropertyValueById
	// (0x10a27d40) reads this and passes it as nReqSkillId when calling
	// GetBuffedProperty, which is what makes multi-row skills (Jetpack Power,
	// Heavy Impact, Super Engineer, …) deliver only the row matching the firing
	// device's class. AssemblyDatManager::CreateDevice doesn't populate this
	// field — that's likely a stripped path on this binary — so we set it
	// directly. Without this, the binary's GetBuffedProperty queries with
	// skillId=0, which means stored>0 entries (the multi-row skill rows) never
	// match → skills silently no-op.
	Device->m_nSkillId = DeviceCategorySkill::Lookup(deviceId);

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

	// --- Morale-device threshold init (was UC `TgDevice_Morale.SetRequiredMoralePoints`,
	// stripped on this binary). Without this every pawn keeps the UC default
	// r_fRequiredMoralePoints=100. The morale device's primary fire mode carries
	// the real value as prop 318 ("Required Points To Fire"); the fire mode's
	// GetPropertyValueById path also folds in any prop 357 ("Required Morale
	// Points Modifier") buff registry entries from skills/mod-kits, so this
	// reads the right post-modifier value.
	//
	// Native binary call: TgDeviceFire::GetRequiredMoralePoints @ 0x10a19910
	// (vtable[0x134] does the GetPropertyValueById(318, fire->s_nRequiredMoralePointsIndex)).
	if (GetDeviceType(slot) == GA_G::TGDT_MORALE && Device != nullptr) {
		typedef float (__fastcall* GetRequiredMoralePointsFn)(UTgDeviceFire*, void*);
		static const GetRequiredMoralePointsFn GetRequiredMoralePointsNative =
			(GetRequiredMoralePointsFn)0x10a19910;

		if (Device->m_FireMode.Num() > 0 && Device->m_FireMode.Data != nullptr) {
			UTgDeviceFire* fire = Device->m_FireMode.Data[0];
			if (fire != nullptr) {
				const float required = GetRequiredMoralePointsNative(fire, nullptr);
				if (required > 0.0f) {
					Pawn->r_fRequiredMoralePoints = required;
					Logger::Log("morale",
						"Equip: pawn=0x%p morale device=%d slot=%d  r_fRequiredMoralePoints -> %.2f\n",
						Pawn, deviceId, slot, required);
				}
			}
		}
	}

	// --- Replication flags ---
	Device->Instigator = (APawn*)Pawn;
	Device->Role = 3;
	Device->RemoteRole = 1;
	Device->bNetInitial = 1;
	Device->bNetDirty = 1;
	Device->bSkipActorPropertyReplication = 0;
	Device->bOnlyDirtyReplication = 0;

	// Inventory.uc defaults: bHidden=true, bReplicateMovement=false. TgDevice
	// inherits both. We DELIBERATELY do NOT override bReplicateMovement here
	// — earlier shape forced it to 1, which made the device's absolute
	// Location/Rotation replicate to clients. CreateEquipDevice spawns the
	// device without ever setting its transform, so it stays at whatever
	// world point the engine's SpawnActor picked (observed: ~ (-4038, -448,
	// 666)) and replicates that stale point forever. Result on remote
	// clients: the AI's Device actor (and weapon mesh attached to it via
	// c_DeviceForm fallback paths) shows at a fixed world point instead of
	// following the pawn — the "AI weapon strangely misplaced" symptom.
	//
	// With bReplicateMovement=false the device's transform is server-only
	// and the client renders the visible weapon via c_DeviceForm attached
	// to the pawn's mesh socket — which is the correct UE3 inventory pattern.
	Device->bHidden = 1;

	// --- Apply permanent equip-effect groups (DB-driven) ---
	// Stand-in for the stripped asm.dat → device->m_EquipEffect path. Most
	// importantly, this is what gives every pawn the +30 physical protection
	// baseline via device 864 (HUMAN BASE ATTRIBUTES).
	ApplyDeviceEquipEffects(Pawn, deviceId);

	// --- Apply rolled mods (the [letters] suffix) ---
	// effect_group_ids stored on UTgInventoryObject::m_nStateEffectGroupIdArray
	// in the live game; we read them from ga_character_devices.mod_effect_group_ids
	// here. Each one targets a property like AOE Radius / Recharge / Healing,
	// applied as additive or percent modifier on the pawn's s_Properties.
	//
	// Pass `invId` so each entry is tagged with the device's instance id,
	// scoping the mod to this device only. See ApplyRolledModEffects header
	// for the per-device-scoping rationale.
	ApplyRolledModEffects(Pawn, deviceId, invId, mods);

	// --- Mirror rolled mods onto the inventory object so server-side UC reads
	// see them too. The TArray is freshly constructed (Data=NULL, Count=0) by
	// ConstructInventoryObject, so a plain Add() per entry is safe.
	if (!mods.empty() && InvObj != nullptr) {
		for (int egid : mods) InvObj->m_nStateEffectGroupIdArray.Add(egid);
	}

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
