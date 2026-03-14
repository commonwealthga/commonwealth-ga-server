#include "src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.hpp"
#include "src/GameServer/TgGame/TgGame/SpawnBotById/TgGame__SpawnBotById.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/TcpServer/TcpEvents/TcpEvents.hpp"
#include "src/Utils/Logger/Logger.hpp"

ATgDevice* __fastcall TgInventoryManager__NonPersistAddDevice::Call(ATgInventoryManager* InventoryManager, void* edx, int nDeviceId, int nEquipPoint) {
	Logger::Log("debug", "MINE TgInventoryManager::NonPersistAddDevice START - deviceId=%d equipPoint=%d\n", nDeviceId, nEquipPoint);

	ATgPawn* ownerpawn = (ATgPawn*)InventoryManager->Owner;
	if (!ownerpawn || !ownerpawn->PlayerReplicationInfo) return nullptr;

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
	int nInventoryId = ++TgGame__SpawnBotById::nInventoryIdCounter;

	// Diagnostics: state before GiveDeviceById
	void* controller = *(void**)((char*)ownerpawn + 0x1D8);
	void* invManager = *(void**)((char*)ownerpawn + 0x39C);
	int instanceIdBefore = ownerpawn->r_EquipDeviceInfo[nEquipPoint].nDeviceInstanceId;
	Logger::Log("debug", "  Controller=0x%p InvManager=0x%p r_EquipDeviceInfo[%d].instanceId BEFORE=%d\n",
		controller, invManager, nEquipPoint, instanceIdBefore);

	TgGame__SpawnBotById::GiveDeviceById(
		ownerpawn,
		(ATgRepInfo_Player*)ownerpawn->PlayerReplicationInfo,
		nDeviceId,
		slotValueId,
		nEquipPoint,
		1162,                    // qualityValueId (default)
		0,                       // isOffHand
		1,                       // isHandDevice — auto-switch to it on pickup
		GA_G::TGDT_PLAYER_SENSOR,// deviceType (beacon/sensor slot)
		nInventoryId
	);

	// Diagnostics: state after GiveDeviceById
	ATgDevice* device = ownerpawn->m_EquippedDevices[nEquipPoint];
	int instanceIdAfter = ownerpawn->r_EquipDeviceInfo[nEquipPoint].nDeviceInstanceId;
	int deviceInstanceId = device ? device->r_nDeviceInstanceId : -1;
	Logger::Log("debug", "  r_EquipDeviceInfo[%d].instanceId AFTER=%d  device->r_nDeviceInstanceId=%d  nInventoryId=%d\n",
		nEquipPoint, instanceIdAfter, deviceInstanceId, nInventoryId);
	// If instanceIdAfter == nInventoryId: UpdateClientDevices fired and detected mismatch (good)
	// If instanceIdAfter == 0:            UpdateClientDevices did NOT fire (guard condition failed)
	// If instanceIdAfter == instanceIdBefore and != 0: slot was already occupied

	// Queue a TCP inventory-add packet so the client's device bar updates.
	auto sessIt = GPawnSessions.find(ownerpawn);
	if (sessIt != GPawnSessions.end()) {
		BeaconPickupEvent bev;
		bev.nPawnId      = ownerpawn->r_nPawnId;
		bev.nDeviceId    = nDeviceId;
		bev.nInventoryId = nInventoryId;
		bev.nEquipSlotValueId = slotValueId;
		GBeaconPickupEvents[sessIt->second].push_back(bev);
		Logger::Log("debug", "  Queued BeaconPickupEvent for session %s (pawnId=%d deviceId=%d invId=%d slot=%d)\n",
			sessIt->second.c_str(), bev.nPawnId, bev.nDeviceId, bev.nInventoryId, bev.nEquipSlotValueId);
	} else {
		Logger::Log("debug", "  WARNING: no session found for pawn 0x%p — TCP device bar will not update\n", ownerpawn);
	}

	// If this is a beacon pickup, immediately unregister the deployed beacon so the entrance
	// deactivates at once rather than waiting up to 1s for CheckBeacon's timer to notice.
	ATgRepInfo_Player* pawnrep = (ATgRepInfo_Player*)ownerpawn->PlayerReplicationInfo;
	if (device && pawnrep && pawnrep->r_TaskForce) {
		ATgTeamBeaconManager* beaconMgr = pawnrep->r_TaskForce->r_BeaconManager;
		if (beaconMgr && beaconMgr->r_Beacon && beaconMgr->r_Beacon->m_nPickupDeviceId == nDeviceId) {
			Logger::Log("debug", "  Beacon pickup detected — unregistering beacon 0x%p from BeaconManager 0x%p\n",
				beaconMgr->r_Beacon, beaconMgr);
			beaconMgr->UnRegisterBeacon(beaconMgr->r_Beacon);
			beaconMgr->bNetDirty      = 1;
			beaconMgr->bForceNetUpdate = 1;
		}
	}

	Logger::Log("debug", "MINE TgInventoryManager::NonPersistAddDevice END - device=0x%p\n", device);
	return device;
}

