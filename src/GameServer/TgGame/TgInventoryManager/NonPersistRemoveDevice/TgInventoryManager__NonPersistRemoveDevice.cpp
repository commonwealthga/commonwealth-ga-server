#include "src/GameServer/TgGame/TgInventoryManager/NonPersistRemoveDevice/TgInventoryManager__NonPersistRemoveDevice.hpp"
#include "src/GameServer/Storage/PawnSessions/PawnSessions.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgInventoryManager__NonPersistRemoveDevice::Call(
	ATgInventoryManager* InventoryManager, void* edx, int nEquipPoint)
{
	if (!InventoryManager) return;
	if (nEquipPoint < 1 || nEquipPoint > 24) return;

	ATgPawn* pawn = (ATgPawn*)InventoryManager->Owner;
	if (!pawn) return;

	ATgDevice* device = pawn->m_EquippedDevices[nEquipPoint];
	if (!device) {
		Logger::Log(GetLogChannel(), "NonPersistRemoveDevice: no device at slot %d\n", nEquipPoint);
		return;
	}

	int nInventoryId = device->r_nInventoryId;
	int nDeviceId    = device->r_nDeviceId;

	Logger::Log(GetLogChannel(), "NonPersistRemoveDevice: slot=%d deviceId=%d invId=%d\n",
		nEquipPoint, nDeviceId, nInventoryId);

	// Null the equip slot. UpdateClientDevices sees null device + stale r_EquipDeviceInfo
	// → clears it and fires the client equip-update handler (replicates via UE3 networking).
	pawn->m_EquippedDevices[nEquipPoint] = nullptr;

	// Mark pawn dirty so the replication system picks up the change.
	*(unsigned int*)((char*)pawn + 0xB0) |= 0x100;
	*(unsigned int*)((char*)pawn + 0xAC) |= 0x100000;

	// Drive the equip-update handler: clears r_EquipDeviceInfo and calls virtual 0x4DC.
	using UpdateFn = void(__fastcall*)(ATgPawn*, void*, int, int);
	((UpdateFn)0x109c9a80)(pawn, nullptr, 0, 0);

	// Decrement r_ItemCount to stay in sync with m_InventoryMap entry count.
	if (InventoryManager->r_ItemCount > 0)
		InventoryManager->r_ItemCount--;

	// Send GAME_EVENT beacon_remove IPC so the control server clears the client's device bar slot.
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
		Logger::Log(GetLogChannel(), "NonPersistRemoveDevice: Sent GAME_EVENT beacon_remove for session %s\n",
			sessIt->second.c_str());
	} else {
		Logger::Log(GetLogChannel(), "NonPersistRemoveDevice: WARNING no session for pawn 0x%p\n", pawn);
	}
}
