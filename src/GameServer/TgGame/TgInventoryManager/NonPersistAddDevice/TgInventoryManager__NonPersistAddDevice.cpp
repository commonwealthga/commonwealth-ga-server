#include "src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

ATgDevice* __fastcall TgInventoryManager__NonPersistAddDevice::Call(ATgInventoryManager* InventoryManager, void* edx, int nDeviceId, int nEquipPoint) {
	Logger::Log("debug", "MINE TgInventoryManager::NonPersistAddDevice START - deviceId=%d equipPoint=%d\n", nDeviceId, nEquipPoint);

	ATgPawn* ownerpawn = (ATgPawn*)InventoryManager->Owner;
	if (!ownerpawn || !ownerpawn->PlayerReplicationInfo) return nullptr;

	// Diagnostic — pickup-mechanism investigation (research REPORT §3.1).
	// Medical station has pickup_device_id=0 in DB so walk-up pickup should
	// be refused, but players report the station becoming pickupable. One
	// candidate: the client sends this add-device RPC for the station's
	// offhand device id and we grant it here unconditionally. Log the
	// current task-force beacon's pickup id so we can compare against the
	// requested device id — if they differ, we're granting a non-beacon
	// device from a client request, which is the bug.
	ATgRepInfo_Player* diagPR = (ATgRepInfo_Player*)ownerpawn->PlayerReplicationInfo;
	int diagBeaconPickupId = -1;
	if (diagPR && diagPR->r_TaskForce && diagPR->r_TaskForce->r_BeaconManager &&
		diagPR->r_TaskForce->r_BeaconManager->r_Beacon) {
		diagBeaconPickupId = diagPR->r_TaskForce->r_BeaconManager->r_Beacon->m_nPickupDeviceId;
	}
	Logger::Log("debug",
		"NonPersistAddDevice[diag]: requested deviceId=%d  current beacon pickup_device_id=%d  match=%d\n",
		nDeviceId, diagBeaconPickupId, (int)(nDeviceId == diagBeaconPickupId));

	// Reverse map: equip point → slot value ID (matches BotGetEquipPointBySlot in SpawnBotById)
	static const int kEquipPointToSlotValueId[25] = {
		0,    // 0 = EQP_NONE
		221,  // 1
		198,  // 2
		200,  // 3
		199,  // 4
		201,  // 5
		202,  // 6
		203,  // 7
		204,  // 8
		385,  // 9
		386,  // 10
		499,  // 11  ← beacon pickup lands here (GetPickupEquipPoint returns 11)
		500,  // 12
		501,  // 13
		502,  // 14
		823,  // 15
		996,  // 16
		997,  // 17
		998,  // 18
		999,  // 19
		1000, // 20
		1001, // 21
		1002, // 22
		1003, // 23
		1004, // 24
	};
	int slotValueId = (nEquipPoint >= 1 && nEquipPoint <= 24) ? kEquipPointToSlotValueId[nEquipPoint] : 0;

	// Each pickup gets a fresh monotonic id. The old hardcoded `999` dates
	// back to the very first inventory-less implementation; it survived this
	// long because deploy→pickup→deploy cycles didn't actually clean up the
	// prior beacon's inventory entry (so reusing the same id "looked" right
	// to the client). With proper cleanup wired via the DeviceFiring.EndState
	// hook (beacon_remove IPC sent on every deploy), each cycle is now:
	//   pickup  → SEND_INVENTORY state=1 with NEW invId → client adds entry
	//   deploy  → SEND_INVENTORY state=2 with that invId → client removes it
	//   pickup  → SEND_INVENTORY state=1 with another NEW invId → ...
	// Reusing 999 across cycles also meant two beacon-carrier devices
	// coexisting in memory with the SAME r_nInventoryId, which made
	// id-based lookups in the binary (GetDeviceByInstanceId, etc.) ambiguous
	// and was the leading suspect for the orphan-device-fires-EndState
	// behavior observed during multi-deploy testing.
	int nInventoryId = Inventory::NextId();

	// Diagnostics: state before GiveDeviceById
	void* controller = *(void**)((char*)ownerpawn + 0x1D8);
	void* invManager = *(void**)((char*)ownerpawn + 0x39C);
	int instanceIdBefore = ownerpawn->r_EquipDeviceInfo[nEquipPoint].nDeviceInstanceId;
	Logger::Log("debug", "  Controller=0x%p InvManager=0x%p r_EquipDeviceInfo[%d].instanceId BEFORE=%d\n",
		controller, invManager, nEquipPoint, instanceIdBefore);

	// isHandDevice=0: do NOT force-switch the player's active hand to the
	// beacon on pickup. The client initiates weapon switches via its own
	// input → ServerSetCurrentWeapon RPC; us writing r_eDesiredInHand
	// from the server (which `isHandDevice=1` causes in GiveDeviceById)
	// is an unrequested server-initiated switch that has no clean teardown
	// when the beacon is deployed — the slot becomes empty and the client
	// falls through to melee. Matching the original game: pick up the
	// beacon, your current weapon stays equipped, you tap the beacon
	// hotkey to switch when you want to place it.
	TgGame__SpawnBotById::GiveDeviceById(
		ownerpawn,
		(ATgRepInfo_Player*)ownerpawn->PlayerReplicationInfo,
		nDeviceId,
		slotValueId,
		nEquipPoint,
		1162,                    // qualityValueId (default)
		0,                       // isOffHand
		0,                       // isHandDevice — don't auto-switch on pickup
		GA_G::TGDT_PLAYER_SENSOR,// deviceType (beacon/sensor slot)
		nInventoryId
	);

	// Diagnostics: state after GiveDeviceById
	ATgDevice* device = ownerpawn->m_EquippedDevices[nEquipPoint];
	int instanceIdAfter = ownerpawn->r_EquipDeviceInfo[nEquipPoint].nDeviceInstanceId;
	int deviceInstanceId = device ? device->r_nDeviceInstanceId : -1;
	Logger::Log("debug", "  r_EquipDeviceInfo[%d].instanceId AFTER=%d  device->r_nDeviceInstanceId=%d  nInventoryId=%d\n",
		nEquipPoint, instanceIdAfter, deviceInstanceId, nInventoryId);

	// Force m_bIsBeaconPlacing=1 on every device created via the pickup slot
	// (slot 11). The official binary's AssemblyDatManager::CreateDevice reads
	// this from asm.dat; our path doesn't set it, so the bit stays 0 and:
	//   (a) NonPersistRemoveDevice's IsABeaconPlacingDevice gate skips the
	//       beacon_remove IPC — control server never clears the device bar
	//       slot for the deployed beacon.
	//   (b) UC TgDevice.ConsumeDevice (TgDevice.uc:667) skips the
	//       beaconManager.CheckBeacon() branch — beacon manager state stale
	//       after consume.
	// Setting the bit here mirrors what the original native would have done.
	if (device != nullptr && nEquipPoint == 11) {
		*(uint32_t*)((char*)device + 0x22C) |= 0x10000u;  // m_bIsBeaconPlacing
	}

	// Carrier-visual investigation: dump both Pawn and PRI r_EquipDeviceInfo
	// slot, plus the device's m_bIsBeaconPlacing flag (TgDevice+0x22C bit
	// 0x10000) which IsCarryingBeacon@0x109BE2D0 ultimately gates on.
	ATgRepInfo_Player* pri = diagPR;
	const int pawnDid = ownerpawn->r_EquipDeviceInfo[nEquipPoint].nDeviceId;
	const int pawnIid = ownerpawn->r_EquipDeviceInfo[nEquipPoint].nDeviceInstanceId;
	const int priDid  = pri ? pri->r_EquipDeviceInfo[nEquipPoint].nDeviceId         : -2;
	const int priIid  = pri ? pri->r_EquipDeviceInfo[nEquipPoint].nDeviceInstanceId : -2;
	uint32_t devFlags = device ? *(uint32_t*)((char*)device + 0x22C) : 0u;
	const int isBeaconPlacing = (device && (devFlags & 0x10000u) != 0) ? 1 : 0;
	Logger::Log("debug",
		"  carrier-visual diag: slot=%d  Pawn.eqi={did=%d,iid=%d}  PRI.eqi={did=%d,iid=%d}  device=0x%p m_bIsBeaconPlacing=%d\n",
		nEquipPoint, pawnDid, pawnIid, priDid, priIid, device, isBeaconPlacing);
	// If instanceIdAfter == nInventoryId: UpdateClientDevices fired and detected mismatch (good)
	// If instanceIdAfter == 0:            UpdateClientDevices did NOT fire (guard condition failed)
	// If instanceIdAfter == instanceIdBefore and != 0: slot was already occupied

	// Send GAME_EVENT beacon_pickup IPC so the control server updates the client's device bar.
	auto sessIt = GPawnSessions.find(ownerpawn);
	if (sessIt != GPawnSessions.end()) {
		nlohmann::json ev;
		ev["type"]                = IpcProtocol::MSG_GAME_EVENT;
		ev["subtype"]             = "beacon_pickup";
		ev["instance_id"]         = IpcClient::GetInstanceId();
		ev["session_guid"]        = sessIt->second;
		ev["pawn_id"]             = (int)ownerpawn->r_nPawnId;
		ev["device_id"]           = nDeviceId;
		ev["inventory_id"]        = nInventoryId;
		ev["equip_slot_value_id"] = slotValueId;
		IpcClient::Send(ev.dump());
		Logger::Log("debug", "  Sent GAME_EVENT beacon_pickup for session %s (pawnId=%d deviceId=%d invId=%d slot=%d)\n",
			sessIt->second.c_str(), (int)ownerpawn->r_nPawnId, nDeviceId, nInventoryId, slotValueId);
	} else {
		Logger::Log("debug", "  WARNING: no session found for pawn 0x%p — beacon_pickup IPC not sent\n", ownerpawn);
	}

	// Beacon pickup is handled entirely by the UC chain now that
	// TgDeployable::GetTaskForce is reimplemented (was a `return 0;` stub):
	//   UC PickUpDeployable -> NonPersistAddDevice (this fn) -> caller sets
	//   s_bWasPickedUp -> DestroyIt -> TgDeploy_Beacon.DestroyIt ->
	//   GetTaskForce().GetBeaconManager().UnRegisterBeacon(self)
	// UnRegisterBeacon (intact native @ 0x109ee6f0) clears mgr->r_Beacon
	// and mgr->r_BeaconInfo, which makes GetBeacon() return null and the
	// BeaconEntrance HasExit() check flip to false.

	Logger::Log("debug", "MINE TgInventoryManager::NonPersistAddDevice END - device=0x%p\n", device);
	return device;
}

