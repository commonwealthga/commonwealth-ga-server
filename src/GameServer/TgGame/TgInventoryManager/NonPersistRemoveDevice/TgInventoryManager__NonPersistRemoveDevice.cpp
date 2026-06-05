#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/GameServer/Inventory/Inventory.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Paired teardown for NonPersistAddDevice. We use Inventory::Unequip so the
// teardown shape matches the equip side (s_equipped / s_deviceByInvId entry,
// invObj in the inventory TMap, device actor, replication info). The old
// shape only nulled m_EquippedDevices[slot] and decremented r_ItemCount,
// leaking the rest on every pickup→deploy cycle.
void __fastcall TgInventoryManager__NonPersistRemoveDevice::Call(
	ATgInventoryManager* InventoryManager, void* edx, int nEquipPoint)
{
	if (!InventoryManager) return;
	if (nEquipPoint < 1 || nEquipPoint > 24) return;

	// Owner is engine AActor* (polymorphic). InventoryManager is only ever
	// spawned by a TgPawn in practice, but gate the cast to avoid reading
	// pawn-specific offsets on a non-pawn if Owner is stale/wrong.
	AActor* owner = InventoryManager->Owner;
	if (!ObjectClassCache::ClassNameContains(owner, "TgPawn")) return;
	ATgPawn* pawn = (ATgPawn*)owner;

	ATgDevice* device = pawn->m_EquippedDevices[nEquipPoint];
	if (!device) {
		Logger::Log(GetLogChannel(), "NonPersistRemoveDevice: no device at slot %d\n", nEquipPoint);
		return;
	}

	// Capture identity + beacon-status BEFORE Unequip — it destroys the
	// actor, so `device` is dangling after the call.
	const int nInventoryId = device->r_nInventoryId;
	const int nDeviceId    = device->r_nDeviceId;
	using IsBeaconFn = int(__fastcall*)(ATgDevice*, void*);
	const bool bIsBeacon = ((IsBeaconFn)0x10a19a40)(device, nullptr) != 0;

	Logger::Log(GetLogChannel(),
		"NonPersistRemoveDevice: slot=%d deviceId=%d invId=%d isBeacon=%d\n",
		nEquipPoint, nDeviceId, nInventoryId, (int)bIsBeacon);

	// Full teardown: nulls m_EquippedDevices[slot], zeros PRI's
	// r_EquipDeviceInfo[slot] + dirties replication, removes the invObj from
	// the inventory TMap via TgInventoryManager::RemoveInventoryObjectById
	// (0x10a16190), DestroyActor on the device, drops s_equipped /
	// s_deviceByInvId tracking, and reverses any rolled-mod / equip-effect
	// baselines we'd registered at Equip time (beacon has neither, so those
	// are no-ops here).
	Inventory::Unequip(pawn, nEquipPoint);

	// Drives the equip-update handler on the engine side: with
	// m_EquippedDevices[slot] now null but the pawn's r_EquipDeviceInfo[slot]
	// still holding stale data, UpdateClientDevices zeros that entry and
	// fires vtable[0x4dc] to notify the client. (Inventory::Unequip only
	// touches the PRI's array — UpdateClientDevices handles the pawn's.)
	pawn->UpdateClientDevices(0, 0);

	// Unequip intentionally leaves r_ItemCount alone — on the equip-screen
	// path a trailing handler resets it from the DB pool, so any write here
	// would race. That trailing handler doesn't run on the consume path, so
	// decrement explicitly to stay in sync with the m_InventoryMap entry
	// count.
	if (InventoryManager->r_ItemCount > 0)
		InventoryManager->r_ItemCount--;

	// Tell the control server to push SEND_INVENTORY (state=2) so the
	// client clears the device-bar slot. Gate on IsABeaconPlacingDevice so
	// consume of a non-beacon-placing device (medical station offhand etc.)
	// doesn't wrongly clear a real device-bar slot.
	if (bIsBeacon) {
		auto sessIt = GPawnSessions.find(pawn);
		if (sessIt != GPawnSessions.end()) {
			nlohmann::json ev;
			ev["type"]         = IpcProtocol::MSG_GAME_EVENT;
			ev["subtype"]      = "beacon_remove";
			ev["instance_id"]  = IpcClient::GetInstanceId();
			ev["session_guid"] = sessIt->second;
			ev["pawn_id"]      = (int)pawn->r_nPawnId;
			ev["inventory_id"] = nInventoryId;
			IpcClient::Send(ev.dump());
			Logger::Log(GetLogChannel(),
				"NonPersistRemoveDevice: Sent GAME_EVENT beacon_remove for session %s\n",
				sessIt->second.c_str());
		} else {
			Logger::Log(GetLogChannel(),
				"NonPersistRemoveDevice: WARNING no session for pawn 0x%p\n", pawn);
		}
	}
}
