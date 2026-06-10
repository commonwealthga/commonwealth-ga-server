#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Misc/CAmBot/LoadBotMarshal/CAmBot__LoadBotMarshal.hpp"
#include "src/GameServer/Misc/CAmItem/LoadItemMarshal/CAmItem__LoadItemMarshal.hpp"
#include "src/GameServer/TgGame/TgAIController/InitBehavior/TgAIController__InitBehavior.hpp"
#include "src/GameServer/TgGame/TgInventoryManager/PrepopulateInventoryId/TgInventoryManager__PrepopulateInventoryId.hpp"
#include "src/GameServer/TgGame/TgInventoryObject_Device/ConstructInventoryObject/TgInventoryObject_Device__ConstructInventoryObject.hpp"
#include "src/GameServer/Misc/AssemblyDatManager/CreateDevice/AssemblyDatManager__CreateDevice.hpp"
#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/TgGame/TgPawn/SyncPawnHealth/SyncPawnHealth.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/Database/Database.hpp"
#include "src/Database/SocketCycle/SocketCycle.hpp"
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

// Build a device's per-fire-mode m_Properties (accuracy / range / damage-radius
// / cooldown / …) by running the intact native TgDevice::ApplyDeviceSetup
// (binary 0x10a1dab0). It looks up the asm device row by Device->r_nDeviceId
// (+0x21C), then InstantiateDeviceFire (0x10a26500) constructs each
// UTgDeviceFire and FUN_10a1f3b0 fills its m_Properties + every m_n<X>Index
// cache from asm_data_set_device_mode_properties.
//
// Player-equipped devices get this for free from the native equip lifecycle,
// but bots are created server-side via CreateEquipDevice, which bypasses it —
// leaving m_Properties empty, so UTgDeviceFire::GetPropertyValueById returned 0
// and only worked via a fallback that read pawn-default stats. Running it here
// gives bot weapons their real per-weapon stats and lets the native property
// pipeline resolve without that fallback. See
// .planning/effect-buff-property-rebuild-PROGRESS.md ("RESOLVED LEAD").
//
// ApplyDeviceSetup only builds m_Properties and instantiates the m_EquipEffect
// TEMPLATE; it does NOT apply the equip-effect buff group (that is the separate
// eventApplyEquipEffects), so this does not double-apply
// Inventory::ApplyDeviceEquipEffects.
//
// The SDK ApplyDeviceSetup wrapper trips the `FunctionFlags |= ~0x400`
// corruption bug, so we ProcessEvent the UFunction with a manual
// save / force-FUNC_Native / restore (same idiom as the beacon path below).
static int BuildDeviceFireProperties(ATgDevice* Device) {
	if (Device == nullptr) return -1;
	static UFunction* pFnApplyDeviceSetup = nullptr;
	if (pFnApplyDeviceSetup == nullptr) {
		pFnApplyDeviceSetup = (UFunction*)ObjectCache::Find(
			"Function TgGame.TgDevice.ApplyDeviceSetup");
	}
	if (pFnApplyDeviceSetup == nullptr) return -1;
	const uint32_t origFlags = pFnApplyDeviceSetup->FunctionFlags;
	pFnApplyDeviceSetup->FunctionFlags = origFlags | 0x400;
	struct { uint32_t ReturnValue; } parms = { 0 };
	Device->ProcessEvent(pFnApplyDeviceSetup, &parms, nullptr);
	pFnApplyDeviceSetup->FunctionFlags = origFlags;
	return (int)parms.ReturnValue;
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
		// Stand-in for the stripped m_EquipEffect path. Mirrors what
		// Inventory::Equip() does — necessary here because this bot path
		// bypasses Inventory::Equip and goes straight to CreateEquipDevice.
		Inventory::ApplyDeviceEquipEffects(Pawn, deviceId);

		Device->s_InventoryObject = InventoryObject;
		Device->r_nDeviceInstanceId = nInventoryId;  // non-zero so UpdateClientDevices detects the change
		Device->m_bIsOffHand = isOffHand;
		Device->m_bHandDevice = isHandDevice;
		Device->m_nDeviceType = deviceType;
		Device->r_nDeviceId = deviceId;
		Device->r_nQualityValueId = qualityValueId;

		// Build the device's fire-mode m_Properties via the native
		// ApplyDeviceSetup (see BuildDeviceFireProperties). Device 1918 already
		// runs ApplyDeviceSetup inside its beacon-equip-effect block below, so
		// skip it here to avoid rebuilding the fire-mode array twice.
		if (deviceId != 1918) {
			BuildDeviceFireProperties(Device);
		}

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

		// Replicate device actor (was server-only as a perf optimization).
		// See the matching block below in GiveDevicesFromBotConfig for the
		// rationale: bots can be possessed by players via -possess or the
		// in-game hacking ability, and possession requires the device
		// NetGUID be registered on the client's connection at the moment
		// of Possess so that ClientWeaponSet's `Instigator != None` check
		// passes. Pre-replicating at spawn makes this deterministic.
		// Device->Role        = 3;
		// Device->RemoteRole  = 0;
		// Device->bNetInitial = 1;
		// Device->NetPriority = 0.5;
		// Device->NetUpdateFrequency = 1;
		// Device->bNetDirty   = 1;
		// Device->bSkipActorPropertyReplication = 0;
		// Device->bOnlyDirtyReplication = 0;
		// Device->bReplicateMovement = 0;
		// Device->bReplicateInstigator = 0;
		// Device->bReplicateRigidBodyLocation = 0;
		// Device->bOnlyRelevantToOwner = 1;
		// Device->bHidden = 1;
		// Inherit Inventory.uc defaults bReplicateMovement=false — see
		// `Inventory::Equip` for the rationale (don't replicate the device's
		// stale spawn-time transform; visible mesh is c_DeviceForm).
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

		// Carrier-visual (the floating ring of symbols) for beacon pickup:
		// device 1918's equip-effect group 5716 has target_fx_id=525 but ZERO
		// rows in asm_data_set_effects, so our existing
		// Inventory::ApplyDeviceEquipEffects (which only walks effects and
		// mutates s_Properties) does nothing for it. The visual side comes
		// from UC's `ApplyEquipEffects` event, which calls
		// `Pawn.ProcessEffect(m_EquipEffect, false, Pawn)` → spawns a
		// TgEffectGroup actor on the pawn → replicates target_fx_id to the
		// client → the ring of symbols renders.
		//
		// For this to work `Device->m_EquipEffect` must be populated, which
		// is `ApplyDeviceSetup`'s job (it reads the device's equip-effect
		// group ref from asm.dat and instantiates the TgEffectGroup
		// template). CreateEquipDevice does NOT call ApplyDeviceSetup, so
		// we invoke it explicitly here.
		//
		// Gated on deviceId == 1918 to keep blast radius narrow — other
		// devices going through this function (bot loadouts) already work
		// via the existing property-mutation path, and triggering
		// eventApplyEquipEffects for them would double-apply any prop
		// changes (the proper TgEffectManager path AND our direct s_Properties
		// writes). If we later prove other devices need the visual too, we
		// remove the gate and drop Inventory::ApplyDeviceEquipEffects.
		//
		// ApplyDeviceSetup is a native — its SDK wrapper has the
		// `pFn->FunctionFlags |= ~0x400` corruption bug (see BeaconSdkSafe).
		// We bypass it with a manual save/force-FUNC_Native/restore ProcessEvent.
		if (deviceId == 1918) {
			static UFunction* pFnApplyDeviceSetup = nullptr;
			if (pFnApplyDeviceSetup == nullptr) {
				pFnApplyDeviceSetup = (UFunction*)ObjectCache::Find(
					"Function TgGame.TgDevice.ApplyDeviceSetup");
			}
			if (pFnApplyDeviceSetup) {
				const uint32_t origFlags = pFnApplyDeviceSetup->FunctionFlags;
				pFnApplyDeviceSetup->FunctionFlags = origFlags | 0x400;
				struct { uint32_t ReturnValue; } parms = { 0 };
				Device->ProcessEvent(pFnApplyDeviceSetup, &parms, nullptr);
				pFnApplyDeviceSetup->FunctionFlags = origFlags;
				// Diagnose ProcessEffect's visual gate: requires
				// (lifetime>0 OR m_bIsManaged) AND (m_bHasVisual AND bEffectApplied).
				// Force-set m_bIsManaged + m_bHasVisual on the template — the
				// native ApplyDeviceSetup reads asm.dat but does NOT set these
				// flags for equip-effect groups (effect_group_type=261). Same
				// idiom our BuildEffectGroup uses for fire-mode templates
				// (reference_effect_system.md).
				if (Device->m_EquipEffect != nullptr) {
					UTgEffectGroup* eg = Device->m_EquipEffect;
					uint32_t pre = *(uint32_t*)((char*)eg + 0x74);
					*(uint32_t*)((char*)eg + 0x74) = pre | 0x01u | 0x80u;  // m_bIsManaged + m_bHasVisual
					uint32_t post = *(uint32_t*)((char*)eg + 0x74);
					Logger::Log("debug",
						"  ApplyDeviceSetup ret=%u  m_EquipEffect=0x%p egId=%d lifetime=%.2f applType=%d cat=%d fxApplied=%d flags74 pre=0x%08x post=0x%08x\n",
						parms.ReturnValue, eg, eg->m_nEffectGroupId, eg->m_fLifeTime,
						eg->m_nApplicationType, eg->m_nCategoryCode, eg->m_nFxAppliedId,
						pre, post);
				} else {
					Logger::Log("debug",
						"  ApplyDeviceSetup ret=%u  m_EquipEffect=NULL\n", parms.ReturnValue);
				}
			}
			// ApplyEquipEffects has three gates:
			// (1) !m_bEquipEffectsApplied  (2) !m_bEffectsOnlyInHand || Instigator.Weapon==self  (3) m_EquipEffect != none
			// (2) is dangerous: Pawn->Weapon doesn't switch to the beacon until
			// the client requests SetCurrentWeapon — server-side it stays as the
			// prior weapon. If m_bEffectsOnlyInHand is true on device 1918, our
			// manual call would skip ProcessEffect.
			//
			// Force m_bEffectsOnlyInHand=0 on this specific device to guarantee
			// gate (2) passes regardless of Weapon state.
			{
				uint32_t devFlags = *(uint32_t*)((char*)Device + 0x22C);
				int bEquipEffectsApplied = (devFlags & 0x4) != 0;
				int bEffectsOnlyInHand   = (devFlags & 0x100) != 0;
				void* pawnWeapon = *(void**)((char*)Pawn + 0x3A0);
				// Clear m_bEffectsOnlyInHand (bit 0x100) so gate (2) passes.
				*(uint32_t*)((char*)Device + 0x22C) = devFlags & ~0x100u;
				void* effMgr = Pawn->r_EffectManager;
				Logger::Log("debug",
					"  pre-ApplyEquipEffects: Device->Instigator=0x%p Pawn->Weapon=0x%p (==Device:%d) bEffectsOnlyInHand=%d->0 bEquipEffectsApplied=%d CurrentFireMode=%d Pawn->r_EffectManager=0x%p\n",
					Device->Instigator, pawnWeapon, (int)(pawnWeapon == Device),
					bEffectsOnlyInHand, bEquipEffectsApplied,
					(int)Device->CurrentFireMode, effMgr);
			}
			// eventApplyEquipEffects is a UC event — SDK wrapper is safe.
			// Guarded internally by m_bEquipEffectsApplied so a re-call is a no-op.
			Device->eventApplyEquipEffects();
			Logger::Log("debug",
				"  post-ApplyEquipEffects: devFlags+0x22C=0x%08x  (m_bEquipEffectsApplied=%d)\n",
				*(uint32_t*)((char*)Device + 0x22C),
				(int)((*(uint32_t*)((char*)Device + 0x22C) & 0x4) != 0));
		}

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
		"b.default_slot_value_id, "
		"COALESCE(d.in_hand_device_flag, 0) AS in_hand_device_flag "
		"FROM asm_data_set_bots_data_set_bot_devices bd "
		"LEFT JOIN asm_data_set_bots b ON b.bot_id = bd.bot_id "
		"LEFT JOIN asm_data_set_items i ON i.item_id = bd.device_id "
		"LEFT JOIN asm_data_set_devices d ON d.device_id = bd.device_id "
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
		bool inHandDeviceFlag  = sqlite3_column_int(stmt, 4) != 0;
		if (deviceId == 0) continue;

		int equipPoint = BotGetEquipPointBySlot(slotUsedValueId);
		Logger::Log("debug", "GiveDevicesFromBotConfig: deviceId=%d slotUsedValueId=%d equipPoint=%d\n", deviceId, slotUsedValueId, equipPoint);
		ATgDevice* Device = Bot->CreateEquipDevice(0, deviceId, equipPoint);
		Logger::Log("debug", "GiveDevicesFromBotConfig: CreateEquipDevice returned 0x%p\n", Device);
		if (Device != nullptr) {
			// Stand-in for the stripped m_EquipEffect path — applies permanent
			// equip-effect groups (e.g. device 864's +30 physical protection).
			Inventory::ApplyDeviceEquipEffects(Bot, deviceId);

			int invId = Inventory::NextId();
			Device->r_nDeviceInstanceId = invId;
			Device->r_eEquippedAt = equipPoint;

			// Hand-vs-offhand classification. `asm_data_set_devices.in_hand_device_flag`
			// distinguishes weapons that must be in-hand to fire (melee, ranged,
			// specialty — flag=1) from off-hand devices that fire without
			// switching the main weapon (grenades, bombs, deployers — flag=0).
			//
			// Without these flags set, the AI's "use device" path treats every
			// device as a regular in-hand weapon. For the assassin specifically,
			// that meant the EMP bomb in ES7 (slot 203, in_hand_device_flag=0)
			// silently failed to fire — the fire path tries to bring it in-hand
			// first, can't (the device's CDO refuses), and the action aborts.
			// The melee equivalent (action 10902 DRIVE BY - HUMAN, slot 221)
			// works fine because slot 221's device IS an in-hand weapon, so
			// the default zero on m_bHandDevice didn't matter as much.
			//
			// Mirror what `GiveDeviceById` (player-equip path) does at line 102-103.
			Device->m_bIsOffHand  = !inHandDeviceFlag;
			Device->m_bHandDevice = inHandDeviceFlag;

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

			// Build the device's fire-mode m_Properties via the native
			// ApplyDeviceSetup so this bot's weapon resolves real per-weapon
			// stats (accuracy/range/…) through the native property pipeline,
			// instead of the empty-m_Properties → pawn-default fallback. The
			// asm lookup keys on r_nDeviceId, which CreateEquipDevice does not
			// reliably set here, so set it first. See BuildDeviceFireProperties.
			Device->r_nDeviceId = deviceId;
			BuildDeviceFireProperties(Device);
			// Replicate the device actor to all clients. The original perf
			// optimization set RemoteRole=0 because "bot weapon visuals come
			// from r_EquipDeviceInfo on the pawn/PRI, not the device actor."
			// That's true for the rendering visuals, but the device actor's
			// NetGUID must be registered on a client's connection BEFORE
			// that client can ever take over the bot (via -possess or the
			// in-game hacking ability). When we tried to enable replication
			// at possess time, ClientWeaponSet fired with an unresolved
			// NetGUID and the weapon-fire path silently stayed broken on
			// the client (the user verified this empirically: a second
			// -possess after the first works, because by then the device's
			// NetGUID has been registered). Enabling replication at spawn
			// pays the cost up-front but makes possession deterministic.
			Device->Role = 3;
			// Device->RemoteRole = 0;
			// Device->bNetInitial = 1;
			// Device->NetPriority = 0.5;
			// Device->NetUpdateFrequency = 1;
			// Device->bNetDirty = 1;
			// Device->bSkipActorPropertyReplication = 0;
			// Device->bOnlyDirtyReplication = 0;
			// Device->bReplicateMovement = 0;
			// Device->bReplicateInstigator = 0;
			// Device->bReplicateRigidBodyLocation = 0;
			// Device->bOnlyRelevantToOwner = 0;
			// Device->SetOwner((AActor*)Bot);
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
	// Bot->bForceNetUpdate = 1;
	// BotRepInfo->bNetDirty = 1;
	// BotRepInfo->bForceNetUpdate = 1;
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
	Logger::Log("pet_spawn", "SpawnBotById: nBotId=%d ignoreCollision=%d loc=(%.1f,%.1f,%.1f)\n",
		nBotId, (int)bIgnoreCollision, vLocation.X, vLocation.Y, vLocation.Z);

	if (pFactory != nullptr && (pFactory->nPriority == 0 || pFactory->nPriority != Game->s_nCurrentPriority)) {
		return nullptr;
	}

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

	if (AIController == nullptr) {
		if (Logger::IsChannelEnabled("pet_spawn")) {
			Logger::Log("pet_spawn", "SpawnBotById: AIController spawn FAILED (TgAIControllerClass=%p)\n",
				(void*)ClassPreloader::GetTgAIControllerClass());
		}
		return nullptr;
	}

	// Look up BotConfig and wire m_pBot.Dummy BEFORE spawning the bot pawn.
	//
	// Once `AIController->Possess(Bot, ...)` runs it sets `Bot->Controller`,
	// which makes the new pawn fully visible to AI scanning natives elsewhere
	// in the world. One such scan is `TgPawn::IsCrewable` (vtable[0x71c]) —
	// the native body reads `pawn->Controller->m_pBot` (offset 0x73c) and
	// dereferences `[m_pBot + 0x5c]` (BOT_TYPE_VALUE_ID) to detect crewable
	// vehicles. If `m_pBot` is still null at that moment we segfault.
	//
	// Previously we assigned `AIController->m_pBot.Dummy = BotConfig` only
	// after `Possess` + `ApplyPawnSetup` (around line 609). That window is
	// safe at map start (no other AI exists yet) but crashes mid-game when a
	// nearby AI controller scans the freshly-spawned bot. Setting it up here
	// closes the window. Also lets us early-out cleanly if BotConfig is
	// missing instead of leaking a half-initialized pawn.
	if (!CAmBot__LoadBotMarshal::m_BotPointers[nBotId]) {
		Logger::Log("pet_spawn", "SpawnBotById: BotConfig pointer missing for nBotId=%d — no assembly.dat row loaded\n", nBotId);
		return nullptr;
	}
	void* BotConfig = (void*)CAmBot__LoadBotMarshal::m_BotPointers[nBotId];
	AIController->m_pBot.Dummy = BotConfig;

	const char* pawnClassName = GetPawnClassName(nBotId);
	UClass* pawnClass = ClassPreloader::GetClass(pawnClassName);
	// DB-resolved class may be defined in a package that isn't loaded server-
	// side (e.g. TgPawn_TurretPlasma lives in a sub-package only loaded for
	// specific maps). Fall back to the base class along the known inheritance
	// chain before giving up. Order matches the SDK inheritance in
	// TgGame_classes.h: TurretX → TgPawn_Turret → TgPawn_Character.
	if (pawnClass == nullptr && strstr(pawnClassName, "TgPawn_Turret") != nullptr) {
		Logger::Log("pet_spawn", "SpawnBotById: '%s' not loaded, falling back to Class TgGame.TgPawn_Turret\n", pawnClassName);
		pawnClassName = "Class TgGame.TgPawn_Turret";
		pawnClass = ClassPreloader::GetClass(pawnClassName);
	}
	if (pawnClass == nullptr) {
		Logger::Log("pet_spawn", "SpawnBotById: fallback '%s' also not loaded, falling back to Class TgGame.TgPawn_Character\n", pawnClassName);
		pawnClassName = "Class TgGame.TgPawn_Character";
		pawnClass = ClassPreloader::GetClass(pawnClassName);
	}
	Logger::Log("pet_spawn", "SpawnBotById: pawnClassName='%s'  pawnClass=%p  AIController=%p\n",
		pawnClassName, (void*)pawnClass, (void*)AIController);

	TgPawn__InitializeDefaultProps::nPendingBotId = nBotId;
	// Raise the enemy-scaling flag only if the caller actually passed a
	// non-null factory (captured BEFORE the null-fallback at the
	// ActorCache::BotFactory line further down). We do NOT clobber to false
	// on a null pFactory: SpawnObjectiveBot raises this flag explicitly
	// before calling us (bosses have no factory but still want scaling), and
	// blanket-clearing here would defeat that.
	if (pFactory != nullptr) {
		TgPawn__InitializeDefaultProps::bPendingEnemyScaling = true;
	}
	ATgPawn_Character* Bot = (ATgPawn_Character*)Game->Spawn(
		pawnClass,
		AIController->PlayerReplicationInfo,
		FName(),
		vLocation,
		rRotation,
		nullptr,
		bIgnoreCollision ? 1 : 0
	);
	// nPendingBotId is cleared inside InitializeDefaultProps::Call

	if (Bot == nullptr) {
		Logger::Log("pet_spawn", "SpawnBotById: Bot spawn FAILED — pawnClass='%s' (%p)  loc=(%.1f,%.1f,%.1f)\n",
			pawnClassName, (void*)pawnClass, vLocation.X, vLocation.Y, vLocation.Z);
		TgPawn__InitializeDefaultProps::nPendingBotId = 0;
		TgPawn__InitializeDefaultProps::bPendingEnemyScaling = false;
		return nullptr;
	}

	ATgRepInfo_Player* BotRepInfo = reinterpret_cast<ATgRepInfo_Player*>(AIController->PlayerReplicationInfo);

	m_spawnedBotIds[(int)Bot] = nBotId;

	// Activate the dedi-server branch in TgPawn_Boss/Turret/Hover/Robot/Siege
	// .GetWeaponStartTraceLocation by giving the pawn a non-null
	// m_TgSocketOffsetInfo. UC only checks `!= none` (verified across all
	// 6 UC accesses); our hook on GetWeaponStartTraceLocationFromSocketOffsetInfo
	// fully replaces the native that would deref the field. Self-pointer is
	// safe — the only 4 binary refs at offset 0x1268 have all been audited.
	// Currently scoped to Boss Shrike (body_asm_id 794); broaden when adding
	// more multi-cannon bots.
	if (SocketCycle::GetBodyAsmId(Bot) == 794) {
		Bot->m_TgSocketOffsetInfo = reinterpret_cast<UTgSocketOffsetInfo*>(Bot);
	}

	// if (pOwnerPawn != nullptr) {
	// 	AIController->SetOwner(pOwnerPawn);
	// } else {
		AIController->SetOwner(WorldInfo);
	// }

	Bot->r_bIsBot = 1;
	AIController->Pawn = Bot;
	Bot->Controller = AIController;
	Bot->PlayerReplicationInfo = AIController->PlayerReplicationInfo;
	Bot->Role = 3;
	Bot->PlayerReplicationInfo->Role = 3;
	AIController->Role = 3;

	// RemoteRole must be ROLE_SimulatedProxy(1): the client's TgPawn.PostBeginPlay
	// only runs SetLocalPlayer (caches c_LocalPC, needed by IsFriendlyWithLocalPawn)
	// when its Role < ROLE_Authority, and client Role == our RemoteRole.
	Bot->RemoteRole = 1;
	Bot->PlayerReplicationInfo->RemoteRole      = 1;
	Bot->PlayerReplicationInfo->bAlwaysRelevant = 1;

	// r_nPawnId is assigned by UC TgPawn.PostBeginPlay via TgGame.GetNextPawnId()
	// during Spawn() — per-TgGame-instance monotonic counter shared by players
	// and bots. Don't override.

	Bot->r_EffectManager->r_Owner = Bot;
	Bot->r_EffectManager->SetOwner(Bot);
	Bot->r_EffectManager->Base = Bot;
	Bot->r_EffectManager->Role = 3;

	BotRepInfo->bBot = 1;
	BotRepInfo->r_PawnOwner = Bot;
	BotRepInfo->r_ApproxLocation = Bot->Location;

	// if (pOwnerPawn != nullptr) {
	// 	BotRepInfo->SetOwner(pOwnerPawn);
	// } else {
		BotRepInfo->SetOwner(WorldInfo);
	// }

	// if (pOwnerPawn != nullptr) {
	// 	Bot->SetOwner(pOwnerPawn);
	// } else {
		Bot->SetOwner(BotRepInfo);
	// }

	if (pOwnerPawn != nullptr) {
		Bot->r_Owner = pOwnerPawn;
		AIController->m_pOwner = pOwnerPawn;


		// Spawner attribution: which fire mode + device instance produced this
		// bot. Server-only fields (no CPF_Net) but consumed by stripped natives
		// that walk pet lists by spawner identity (e.g. HUD device-bar slot
		// matching, "find my turret for slot N" lookups). Without these, those
		// natives see zero on every bot and silently fall through.
		if (deviceFire) {
			Bot->s_nSpawnerDeviceModeId = deviceFire->m_nId;
			ATgDevice* sourceDevice = (ATgDevice*)deviceFire->m_Owner;
			if (sourceDevice) {
				Bot->s_nSpawnerDeviceInstId = sourceDevice->r_nDeviceInstanceId;
			}
		}

		// HUD device-bar healthbar wiring for pets. The HUD's FindSpawnedPet
		// (0x114de150) walks `localPlayer.PRI.m_PRIArray` for pets and matches
		// by `BotPRI->r_nProfileId == fireMode.bot_id`. Two pieces required:
		//
		//   1. r_nProfileId on the bot's PRI must equal the bot_id (lookup key).
		//
		//   2. m_PRIArray on the OWNER's PRI must contain the bot's PRI. The
		//      population mechanism is replication-driven: setting
		//      BotPRI.r_MasterPrep = ownerPRI (server) → replicates to client →
		//      client UC fires ReplicatedEvent('r_MasterPrep') →
		//      CheckMembership native (0x109f0030) → calls vtable[0x390] on
		//      the new master with this PRI → master's m_PRIArray gets the
		//      bot's PRI appended. (vtable[0x390] is the AddMinion-equivalent
		//      slot per the CheckMembership decompile.)
		//
		// Both fields (r_MasterPrep, r_nProfileId) are CPF_Net + repnotify and
		// already in our optimized rep list, so the client receives them.
		ATgRepInfo_Player* OwnerPRI = (ATgRepInfo_Player*)pOwnerPawn->PlayerReplicationInfo;
		if (BotRepInfo && OwnerPRI) {
			BotRepInfo->r_MasterPrep = OwnerPRI;
			BotRepInfo->r_nProfileId = nBotId;
			//
			// // NOTE: We previously also assigned `Team` and `r_TaskForce` here
			// // to fix the team-color flicker on the deployer's first frame.
			// // That broke the device-bar healthbar — setting r_TaskForce makes
			// // `TgRepInfo_Player::CheckMembership` (0x109f0030) run its second
			// // branch (the r_TaskForce-change path) which does additional
			// // bookkeeping that appears to interfere with the m_PRIArray
			// // membership the FIRST branch establishes. Setting `Team`
			// // triggers stock UE3 APlayerReplicationInfo::SetTeam which
			// // calls AddToTeam/RemoveFromTeam on the TaskForce. Combined,
			// // the BotPRI ends up not in the deployer's m_PRIArray after
			// // the dust settles → HUD's FindSpawnedPet finds nothing → no
			// // healthbar. Leaving these unset; team color will resolve via
			// // r_MasterPrep → CheckMembership chain (with the brief flicker).
			//
			// // Two-knob fix for the team-color flicker on the deployer's client:
			// //
			// // (1) NetPriority bump (8.0 vs TgPawn's 4.0) so within a single
			// // relevance pass BotRepInfo's bunch is serialized before the bot
			// // pawn's. The bot pawn carries a NetGUID reference to its PRI;
			// // we want the PRI's actor channel open on the client first so the
			// // reference resolves immediately instead of hitting the deferred
			// // NetGUID resolution path (which takes ~1 second to settle).
			// //
			// // (2) NetUpdateFrequency bump (10.0 vs APlayerReplicationInfo's
			// // default 1.0). PRI's default 1Hz cadence means after the initial
			// // replication, the engine doesn't re-consider the PRI for ~1
			// // second — exactly the flicker duration. The bot pawn replicates
			// // at 100Hz the whole time. Bumping the PRI frequency lets the
			// // engine re-replicate the BotRepInfo / re-resolve its NetGUID on
			// // the client within a few frames if anything was pending.
			// //
			// // Trace: TgRepInfo_Game::CheckIsEnemy (0x109ee170) compares
			// // taskforce bytes from GRI::GetTaskForceFor(actor) (0x109f1fa0).
			// // For a bot pawn the only path is `pawn->GetPRI()->r_TaskForce`
			// // — null PRI on first frame → both sides resolve to null →
			// // returns "enemy". Once GUID resolution catches up, repnotify on
			// // r_TaskForce fires, NotifyTeamChanged → RecalculateMaterial(true).
			// BotRepInfo->NetPriority        = 5.0f;
			// BotRepInfo->NetUpdateFrequency = 5.0f;
			// BotRepInfo->bNetDirty       = 1;
			// BotRepInfo->bForceNetUpdate = 1;
		}

		// Per-category pet limit: 1 turret + 1 drone allowed simultaneously,
		// deploying a 2nd of the SAME category destroys the prior one.
		// Two slots:
		//   pawn->s_Turret  → TgPawn_Turret subclasses (sentry-style turrets)
		//   pawn->r_Pet     → everything else spawned via SpawnPet (drones)
		// Don't fan the new bot into BOTH slots — that breaks coexistence
		// (deploying a drone while a turret is alive would clobber r_Pet,
		// then UC's `r_Owner.r_Pet == self` check on the drone returns true
		// while the next turret deploy would Suicide it via the wrong slot).
		const bool botIsTurret = (Bot->Class &&
			strstr(Bot->Class->GetFullName(), "TgPawn_Turret") != nullptr);

		if (botIsTurret) {
			ATgPawn* prior = (ATgPawn*)pOwnerPawn->s_Turret;
			if (prior && prior != Bot && !prior->bDeleteMe) {
				Logger::Log("pet_spawn",
					"[PetLimit] suiciding prior turret 0x%p (replaced by 0x%p)\n",
					prior, Bot);
				prior->Suicide();
			}
			pOwnerPawn->s_Turret = (ATgPawn_Turret*)Bot;
		} else {
			ATgPawn* prior = pOwnerPawn->r_Pet;
			if (prior && prior != Bot && !prior->bDeleteMe) {
				Logger::Log("pet_spawn",
					"[PetLimit] suiciding prior drone 0x%p (replaced by 0x%p)\n",
					prior, Bot);
				prior->Suicide();
			}
			pOwnerPawn->r_Pet = Bot;
		}
	}

	AIController->Possess(Bot, 0, 1);

	// PHYS_Flying setup is handled by the ProcessEvent hook on
	// `Function TgGame.TgPawn.PostPawnSetup` (UObject__ProcessEvent.cpp).
	// Writing Physics=4 here would just be clobbered — UC's
	// `TgPawn.PostPawnSetup` does an unconditional `SetPhysics(ROLE_SimulatedProxy)`
	// (= PHYS_Falling, value 2) which runs asynchronously via the
	// `WaitForInventoryThenDoPostPawnSetup` timer, AFTER SpawnBotById returns.
	// The hook applies the right physics after that call completes.

	// Apply DB-derived collision cylinder + auxiliary pawn fields.
	//
	// Replaces what the native SetCollisionFromMesh (0x109be6d0) would have
	// done — but our spawn ordering (r_nBodyMeshAsmId is set further down,
	// after ApplyPawnSetup) means the native fires with id=0 and no-ops, so
	// we are the de-facto cylinder writer. ApplyBotCollisionData does the
	// full setup the native would have: cylinder size (preferring the
	// designer's tighter `hit_collision_*` when set — fixes hover bots like
	// Support Scanner whose collision_* would put the cylinder hanging below
	// the chassis), APawn.CrouchHeight/Radius for engine crouch handling,
	// m_fStandingHeight/Radius for UnCrouch restoration, and
	// m_fTargetCylinderHeight/Radius which other natives consume as the
	// "tight hit body" dimensions.
	TgGame__SpawnBotById::ApplyBotCollisionData(Bot, nBotId);

	Bot->ApplyPawnSetup();
	// Bot->WaitForInventoryThenDoPostPawnSetup();
	Bot->InvManager->Instigator = Bot;

	// BotConfig was already resolved and assigned to AIController->m_pBot.Dummy
	// up top (see the comment block right after AIController spawn). Reuse it
	// here for the pawn-level field; no second lookup needed.
	Logger::Log("pet_spawn", "SpawnBotById: BotConfig resolved, proceeding with post-spawn setup for nBotId=%d Bot=%p\n", nBotId, (void*)Bot);

	Bot->m_pAmBot.Dummy = BotConfig;

	// BotConfig field layout decoded from CGameClient__BotSetup @ 0x1094c730.
	// The original engine's `TgPawn::InitializeDefaultProps` (0x109bf400) was
	// the native that copied these into the spawned pawn; it's stripped to a
	// stub in our build, so we have to do it here. All replicated fields are
	// CPF_Net so the client picks them up automatically.
	Bot->r_nPhysicalType       = *(int*)((char*)BotConfig + 0x64); // PHYSICAL_TYPE_VALUE_ID
	Bot->r_nBodyMeshAsmId      = *(int*)((char*)BotConfig + 0x54); // BODY_ASM_ID

	// Identity / classification — none of these were being set, so every bot
	// looked like a "rank=0, profile=0, hunts=nothing" pawn to AI natives that
	// branch on them (e.g. henchman/prey/rank-based action selection).
	Bot->r_nProfileId          = nBotId;                                  // matches BotConfig+0x1c bot_id
	Bot->r_nProfileTypeValueId = *(int*)((char*)BotConfig + 0x60);        // CLASS_TYPE_VALUE_ID (0 for most bots; Medic/Recon/Assault/Robotics 941-944 only for player profiles)
	Bot->r_nBotRankValueId     = *(int*)((char*)BotConfig + 0x68);        // BOT_RANK_VALUE_ID — group 119: 1048 Elite, 1051 Support, 1052 Minion
	Bot->r_nPreyProfileType    = *(int*)((char*)BotConfig + 0x6c);        // TARGET_ONLY_PHYSICAL_TYPE_VALUE_ID — pairs with target's r_nProfileTypeValueId in IsPreyOfHunter (TgPawn.uc:7318)

	// Death-drop rewards. Currently zero everywhere, so killing a bot gives
	// nothing — XP/currency progression for players is dead until we set these.
	Bot->r_nXp                 = *(int*)((char*)BotConfig + 0xf4);        // XP_VALUE — XP awarded on death
	Bot->r_nCurrency           = *(int*)((char*)BotConfig + 0xf8);        // CURRENCY_VALUE — currency dropped on death

	// Default posture. CGameClient__BotSetup (0x1094c730) marshals the DB's
	// default_posture_value_id (FK group 61) into BotConfig+0xa4 already
	// translated to the TG_POSTURE byte enum via DAT_119a1c80 vtable[0x28].
	// We must propagate that byte onto r_ePosture (offset 0x6F0, CPF_Net)
	// AND mark the field dirty so the initial replication actually fires —
	// otherwise the client's BlendByPostureAnimNode picks the wrong child
	// (or never selects one) and bots visibly settle into a "kinda crouching"
	// idle even though their collision cylinder is full-size. 333/346 bots in
	// the DB have default_posture_value_id=334 (Default → enum 0), so most
	// just get a clean Default-posture announcement; the Hibernate/Patrol/
	// Job1 bots get their actual non-zero starting posture.
	Bot->r_ePosture = (unsigned char)(*(int*)((char*)BotConfig + 0xa4));

	// Emplacement turrets (bot-factory / objective spawned, e.g. firing-range
	// and guard turrets) spawn ALREADY deployed. r_bIsDeployed (0x1C54, CPF_Net)
	// is normally false at spawn, so when a client first becomes relevant to the
	// turret — which happens as the player approaches/LOS — UC
	// OnWaitingForPawnDone -> SetDeployStateBasedOnPosture sees !r_bIsDeployed
	// and runs StartDeploy(), playing the deploy animation right then ("turret
	// deploys when I walk up"). Default posture is already Default(0)=deployed
	// and these bots never hibernate (hibernate_on_idle_sec=0), so pre-seeding
	// r_bIsDeployed=true makes both server and client take the "already up"
	// branch (percentage=1.0, no StartDeploy). Initial replication carries the
	// true value, so no bNetDirty needed.
	//
	// Gate on pOwnerPawn==nullptr: player-deployed turrets (TgDeviceFire::
	// SpawnPet passes the owning player as pOwnerPawn and a non-zero
	// fDeployAnimLength) MUST keep their deploy phase and are left untouched.
	// (pFactory is unreliable here — it gets back-filled from ActorCache below
	// when null, so it can't distinguish player deploys.)
	//
	// Also require r_ePosture==Default(0): a turret bot configured to start
	// hibernating (posture 1) is meant to stay packed, and the client's
	// SetDeployStateBasedOnPosture(posture 1) would force r_bIsDeployed back to
	// false anyway — so only pre-deploy posture-0 (deployed) turrets.
	if (pOwnerPawn == nullptr && Bot->r_ePosture == 0 && pawnClassName &&
	    strstr(pawnClassName, "TgPawn_Turret") != nullptr) {
		((ATgPawn_Turret*)Bot)->r_bIsDeployed = 1;
	}

	// Explicitly clear UE3 crouch state on the bot. APawn::bIsCrouched
	// (offset 0x1EC bit 0x08, CPF_Net) replicates to non-owning clients via
	// the V2 rep block's `bNetDirty && !bNetOwner` gate. If it's true at spawn,
	// every observing client renders the mesh with `m_fCrouchTranslationOffset
	// = -32` translated down via AdjustMeshTranslation (native, fired from
	// StartCrouch) — the mesh looks "low" but the collision cylinder stays
	// full-size. That matches the symptom: boss bots (and several other AI
	// bots) appear crouched while walking. Possessing the bot bypasses this
	// because crewing's UC ProcessEvent path on the player must eventually
	// touch state that uncrouches it (or repathing forces ShouldCrouch(false)),
	// so the user sees normal stance only while controlling.
	//
	// Clear both bIsCrouched and bWantsToCrouch — the engine processes the
	// pending want flag next tick and would re-crouch otherwise.
	*(unsigned int*)((char*)Bot + 0x1EC) &= ~0x0Cu;  // clear bits 0x04 (bWantsToCrouch) + 0x08 (bIsCrouched)


	// s_nCharacterId is defined on ATgPawn_Character (offset 0x162C). Non-
	// Character bots (TgPawn_Hover, TgPawn_Robot, TgPawn_VanityPet, ...) reuse
	// that offset for subclass fields. In particular TgPawn_Hover has a
	// UAnimNodePlayCustomAnim* m_CustomHitWallNode at 0x162C — writing the
	// int BOT_TYPE_VALUE_ID there would corrupt a UObject* pointer and
	// produce the recurring 0x1d5 crashes (VM dispatch via HitWall + GC walk
	// both dereference this field). Only write when the spawned class is
	// actually a Character descendant.
	const bool isCharacter =
		pawnClassName &&
		(strstr(pawnClassName, "TgPawn_Character") ||
		 strstr(pawnClassName, "TgPawn_Boss")      ||
		 strstr(pawnClassName, "TgPawn_NPC")       ||
		 strstr(pawnClassName, "TgPawn_Interact_NPC") ||
		 strstr(pawnClassName, "TgPawn_Sniper")    ||
		 strstr(pawnClassName, "TgPawn_SonoranCommander") ||
		 strstr(pawnClassName, "TgPawn_Turret"));
	if (isCharacter) {
//		Bot->s_nCharacterId = *(int*)((char*)BotConfig + 0x5C); // BOT_TYPE_VALUE_ID
		// Character-subclass replicated fields. Same offset-collision caveat
		// as s_nCharacterId — only safe to write when the actual class is
		// Character-descended.
		*(int*)((char*)Bot + 0x1630) = *(int*)((char*)BotConfig + 0x50); // r_nHeadMeshAsmId ← HEAD_ASM_ID
		*(int*)((char*)Bot + 0x1640) = *(int*)((char*)BotConfig + 0xb4); // r_nSkillGroupSetId ← SKILL_GROUP_SET_ID
	}

	Bot->r_bIsStealthed = 0;

	// PRI is wired by this point — fan HP across all 7 storage locations
	// (engine fields, ATgPawn replicated max, property descriptors, PRI).
	const int botHp = *(int*)((char*)BotConfig + 0x74);
	// SyncPawnHealth::Apply(Bot, botHp, botHp);
	//BotRepInfo->r_nCharacterId    = *(int*)((char*)BotConfig + 0x5C);

	if (pFactory == nullptr) {
		ActorCache::CacheMapActors();
		pFactory = ActorCache::BotFactory;
		if (pFactory && Logger::IsChannelEnabled("debug")) {
			Logger::Log("debug", "TgBotFactory fallback found: %s\n", pFactory->GetFullName());
		}
	} else if (Logger::IsChannelEnabled("debug")) {
		Logger::Log("debug", "TgBotFactory passed: %s\n", pFactory->GetFullName());
	}

	if (pFactory != nullptr && BotRepInfo != nullptr) {
		const int team = (pFactory->s_nTaskForce == 1) ? GTeamsData.Attackers : GTeamsData.Defenders;
		BotRepInfo->r_TaskForce = team;
		BotRepInfo->Team        = team;
		BotRepInfo->SetTeam(team);
		Bot->NotifyTeamChanged();

		// r_bInitialIsEnemy backs the client's friend/enemy fallback during the
		// bot-PRI NetGUID window; the stealth MIC is baked from it at cloak time.
		// Bots spawned before any player joined are fixed up in SpawnPlayerCharacter.
		ATgRepInfo_TaskForce* botTf = (ATgRepInfo_TaskForce*)(intptr_t)team;
		for (const auto& kv : GClientConnectionsData) {
			ATgPawn_Character* hp = kv.second.Pawn;
			ATgRepInfo_Player* hpri = hp ? (ATgRepInfo_Player*)hp->PlayerReplicationInfo : nullptr;
			if (hpri && hpri->r_TaskForce) {
				Bot->r_bInitialIsEnemy = (botTf != hpri->r_TaskForce) ? 1 : 0;
				Bot->bNetDirty = 1;
				break;
			}
		}
	}


	// AIController->m_nSquadRole = 1404;

	// AIController->m_pBot.Dummy was set up-top before bot Spawn — see the
	// IsCrewable race comment in the AIController-spawn block.
	TgAIController__InitBehavior::CallOriginal(AIController, edx, BotConfig, pFactory);

	// Anchor the AIController to the spawn pose. m_vSpawnLocation (0x5F4) is
	// consumed by behavior-tree actions whose destination_type=238 ("SPAWN
	// LOCATION") — e.g. Boss Think-Tank action 11267 "GO TO - SPAWN LOCATION"
	// in behavior 676. Without this write the field defaults to zero and the
	// bot can't anchor itself or recall its origin after chasing. The original
	// game's Kismet spawn flow set this implicitly; we have to do it by hand
	// since most of our spawn paths (objective tier 2/3, /spawnbot, factories
	// without designer-set state) skip the Kismet code that wrote it.
	// m_rSpawnDirection (0x600) is the matching facing — used by anchored
	// behaviors that want the bot to return to its original orientation too.
	AIController->m_vSpawnLocation  = vLocation;
	AIController->m_rSpawnDirection = rRotation;

	// Emplacement turrets (no owner pawn — bot-factory / objective spawned)
	// need their facing pinned across the FULL set of fields the AI idle
	// rotation tick reads, or they snap to north regardless of spawn rotation.
	// Possess() leaves m_rFixedDirection / DesiredRotation at (0,0,0) (world-X
	// = north), and TgAIController's idle branch (uc:2080-2093) reasserts that
	// every tick — so setting m_rSpawnDirection alone (above) isn't enough; the
	// fixed-direction branch wins and faces north. This mirrors the
	// post-spawn rotation pin already used for player-deployed turrets in
	// TgDeviceFire::SpawnPet. Scoped to turrets so moving/patrolling bots keep
	// free rotation. Player-deployed turrets keep their own pin (pOwnerPawn set).
	if (pOwnerPawn == nullptr && pawnClassName &&
	    strstr(pawnClassName, "TgPawn_Turret") != nullptr) {
		Bot->Rotation                   = rRotation;
		Bot->DesiredRotation            = rRotation;
		AIController->Rotation          = rRotation;
		AIController->DesiredRotation   = rRotation;
		AIController->m_rFixedDirection = rRotation;
	}

	// BotRepInfo->bNetDirty = 1;
	// BotRepInfo->bSkipActorPropertyReplication = 0;
	// BotRepInfo->bOnlyDirtyReplication = 0;
	// BotRepInfo->bNetInitial = 1;
	// BotRepInfo->NetPriority = 5;
	// BotRepInfo->NetUpdateFrequency = 5;
	// BotRepInfo->bAlwaysRelevant = 0;

	// Bot->r_EffectManager->RemoteRole = 1;
	// Bot->r_EffectManager->bNetInitial = 1;
	// Bot->r_EffectManager->bNetDirty = 1;

	Bot->r_EffectManager->r_Owner    = (AActor*)Bot;
	Bot->r_EffectManager->SetOwner((AActor*)Bot);
	Bot->r_EffectManager->Base       = (AActor*)Bot;
	Bot->r_EffectManager->Role       = 3;
	Bot->r_EffectManager->RemoteRole = 1;
	// Bot->r_EffectManager->bNetInitial = 1;
	// Deployable->r_EffectManager->bNetDirty   = 1;
	// Bot->r_EffectManager->NetPriority = 0.5;
	// Bot->r_EffectManager->NetUpdateFrequency = 10;

	// Bot->r_EffectManager->bReplicateMovement = 0;
	// Bot->r_EffectManager->bReplicateInstigator = 0;
	// Bot->r_EffectManager->bReplicateRigidBodyLocation = 0;
	// Bot->r_EffectManager->bOnlyRelevantToOwner = 0;


	// Bot->bNetInitial = 1;
	// // Bot->bNetDirty = 1;
	// Bot->bReplicateMovement = 1;
	// Bot->RemoteRole = 1;
	// Bot->bSkipActorPropertyReplication = 0;
	// Bot->NetPriority = 10;
	// Bot->NetUpdateFrequency = 10;

	GiveDevicesFromBotConfig(Bot, BotRepInfo, nBotId);

	return Bot;
}

