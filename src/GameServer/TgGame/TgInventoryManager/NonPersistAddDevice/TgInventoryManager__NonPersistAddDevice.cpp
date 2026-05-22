#include "src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/Constants/EquipSlot.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Beacon pickup path — invoked from UC `PickUpDeployable` after the deployable
// returns its pickup_device_id. We mint a fresh non-persist inventory entry
// and equip it on the picker.
//
// We use Inventory::Equip (the canonical player-equip path) rather than the
// older `TgGame__SpawnBotById::GiveDeviceById` helper. That helper sets
// `RemoteRole=0` (server-only — a perf optimization for bot loadouts that
// was kept in the helper but updated everywhere else). For a player pawn
// that breaks the client: the device actor never replicates, the client
// receives SEND_INVENTORY with the new invId but its NetGUID/actor scan
// finds no matching ATgDevice, and the inventory recovery path loops
// emitting "Resubmitting an inventory item that could not find its device".
//
// Inventory::Equip sets the standard player-device replication shape
// (RemoteRole=1, bNetInitial=1, bNetDirty=1) and registers the entry in
// `s_equipped` / `s_deviceByInvId` so the matching Unequip in
// NonPersistRemoveDevice can tear it down cleanly on consume.
//
// Beacon invIds come from `Inventory::NextId()` (monotonic from 1,000,000+) —
// well outside the player-loadout range (10001..10025), and distinct across
// every carrier so two players (or both teams) holding beacons can't collide.
// Nothing here writes to the DB — the entry lives only as long as the match.
ATgDevice* __fastcall TgInventoryManager__NonPersistAddDevice::Call(
	ATgInventoryManager* InventoryManager, void* edx,
	int nDeviceId, int nEquipPoint)
{
	if (!InventoryManager) return nullptr;
	if (nEquipPoint < 1 || nEquipPoint > 24) return nullptr;

	ATgPawn* ownerpawn = (ATgPawn*)InventoryManager->Owner;
	if (!ownerpawn || !ownerpawn->PlayerReplicationInfo) return nullptr;

	// Diagnostic — pickup-mechanism investigation (research REPORT §3.1).
	// Medical station has pickup_device_id=0 in DB so walk-up pickup should
	// be refused, but players reported the station becoming pickupable.
	// Logged here so if the requested deviceId mismatches the current team
	// beacon's pickup_device_id we can spot a non-beacon pickup leaking
	// through.
	ATgRepInfo_Player* diagPR = (ATgRepInfo_Player*)ownerpawn->PlayerReplicationInfo;
	int diagBeaconPickupId = -1;
	if (diagPR && diagPR->r_TaskForce && diagPR->r_TaskForce->r_BeaconManager &&
		diagPR->r_TaskForce->r_BeaconManager->r_Beacon) {
		diagBeaconPickupId = diagPR->r_TaskForce->r_BeaconManager->r_Beacon->m_nPickupDeviceId;
	}
	Logger::Log("debug",
		"NonPersistAddDevice[diag]: requested deviceId=%d  current beacon pickup_device_id=%d  match=%d\n",
		nDeviceId, diagBeaconPickupId, (int)(nDeviceId == diagBeaconPickupId));

	// quality 1162 mirrors what the prior GiveDeviceById path hardcoded
	// (the equip-screen default for "no quality").
	const int invId = Inventory::NextId();
	ATgDevice* device = Inventory::Equip(ownerpawn, nDeviceId, nEquipPoint,
		/*quality=*/1162, invId, /*mods=*/{});
	if (device == nullptr) {
		Logger::Log("beacon",
			"NonPersistAddDevice: Inventory::Equip returned null pawn=0x%p devId=%d slot=%d\n",
			ownerpawn, nDeviceId, nEquipPoint);
		return nullptr;
	}

	// m_bIsBeaconPlacing — the binary's asm.dat → CreateDevice path normally
	// reads this from data; CreateEquipDevice doesn't. Set it ourselves on
	// the beacon slot so:
	//   (a) NonPersistRemoveDevice's IsABeaconPlacingDevice gate fires
	//       the beacon_remove IPC on consume,
	//   (b) UC TgDevice.ConsumeDevice (TgDevice.uc:667) runs the
	//       beaconManager.CheckBeacon() branch.
	if (nEquipPoint == 11) {
		*(uint32_t*)((char*)device + 0x22C) |= 0x10000u;
	}

	// Carrier-visual (floating ring of symbols around the beacon-holder).
	// Device 1918's equip-effect group 5716 has target_fx_id=525 but ZERO
	// rows in asm_data_set_effects, so Inventory::ApplyDeviceEquipEffects
	// (which only walks effects and mutates s_Properties) does nothing for
	// it. The visual side comes from UC `ApplyEquipEffects` → `Pawn.ProcessEffect(
	// m_EquipEffect, false, Pawn)` → spawns a TgEffectGroup actor → replicates
	// target_fx_id=525 → client renders the ring.
	//
	// For that chain to fire we need three things CreateEquipDevice doesn't
	// do on its own:
	//   1. Device->m_EquipEffect populated  → call native ApplyDeviceSetup
	//      (it reads the equip-effect group ref from asm.dat and instantiates
	//      the TgEffectGroup template).
	//   2. The template's m_bIsManaged + m_bHasVisual flags set — the native
	//      doesn't set them for effect_group_type=261 (equip-effect category).
	//      ProcessEffect's visual gate requires both.
	//   3. m_bEffectsOnlyInHand cleared on the device — server-side
	//      Pawn->Weapon doesn't switch to the beacon until the client requests
	//      SetCurrentWeapon, so gate (2) of ApplyEquipEffects would reject.
	//
	// Gated on deviceId == 1918: other devices flowing through this function
	// already work via direct s_Properties writes, and triggering the proper
	// UC path for them would double-apply prop changes.
	//
	// ApplyDeviceSetup is a native — the SDK wrapper trips the
	// `FunctionFlags |= ~0x400` corruption bug, so we ProcessEvent it with a
	// manual save/force-FUNC_Native/restore.
	if (nDeviceId == 1918) {
		static UFunction* pFnApplyDeviceSetup = nullptr;
		if (pFnApplyDeviceSetup == nullptr) {
			pFnApplyDeviceSetup = (UFunction*)ObjectCache::Find(
				"Function TgGame.TgDevice.ApplyDeviceSetup");
		}
		if (pFnApplyDeviceSetup) {
			const uint32_t origFlags = pFnApplyDeviceSetup->FunctionFlags;
			pFnApplyDeviceSetup->FunctionFlags = origFlags | 0x400;
			struct { uint32_t ReturnValue; } parms = { 0 };
			device->ProcessEvent(pFnApplyDeviceSetup, &parms, nullptr);
			pFnApplyDeviceSetup->FunctionFlags = origFlags;

			if (device->m_EquipEffect != nullptr) {
				// m_bIsManaged | m_bHasVisual on the effect group template.
				*(uint32_t*)((char*)device->m_EquipEffect + 0x74) |= 0x01u | 0x80u;
			}
		}
		// Clear m_bEffectsOnlyInHand so ApplyEquipEffects's in-hand gate
		// passes regardless of Pawn->Weapon.
		*(uint32_t*)((char*)device + 0x22C) &= ~0x100u;

		// eventApplyEquipEffects is a UC event — SDK wrapper is safe.
		// Internally guarded by m_bEquipEffectsApplied so re-calls are no-ops.
		device->eventApplyEquipEffects();
	}

	// Drives UpdateClientDevices + sets pawn/PRI dirty bits — same wrap-up
	// as the loadout-equip path.
	Inventory::Finalize(ownerpawn);

	// Tell the control server to push SEND_INVENTORY (state=1) so the
	// client's device bar shows the new beacon slot. The actor itself
	// replicates via UE3 actor channels because Inventory::Equip wires it
	// for replication.
	auto sessIt = GPawnSessions.find(ownerpawn);
	if (sessIt != GPawnSessions.end()) {
		const int slotValueId = GA::SlotValueId(nEquipPoint);
		nlohmann::json ev;
		ev["type"]                = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]             = "beacon_pickup";
		ev["instance_id"]         = IpcClient::GetInstanceId();
		ev["session_guid"]        = sessIt->second;
		ev["pawn_id"]             = (int)ownerpawn->r_nPawnId;
		ev["device_id"]           = nDeviceId;
		ev["inventory_id"]        = invId;
		ev["equip_slot_value_id"] = slotValueId;
		IpcClient::Send(ev.dump());
		Logger::Log("beacon",
			"NonPersistAddDevice: equipped pawn=%d device=%d slot=%d invId=%d (slotValueId=%d)\n",
			(int)ownerpawn->r_nPawnId, nDeviceId, nEquipPoint, invId, slotValueId);
	} else {
		Logger::Log("beacon",
			"NonPersistAddDevice: WARNING no session for pawn 0x%p — beacon_pickup IPC not sent\n",
			ownerpawn);
	}

	return device;
}
