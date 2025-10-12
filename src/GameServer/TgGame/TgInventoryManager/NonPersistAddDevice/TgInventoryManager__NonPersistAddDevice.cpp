#include "src/GameServer/TgGame/TgInventoryManager/NonPersistAddDevice/TgInventoryManager__NonPersistAddDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

ATgDevice* __fastcall TgInventoryManager__NonPersistAddDevice::Call(ATgInventoryManager* InventoryManager, void* edx, int nDeviceId, int nEquipPoint) {
	// LogToFile("C:\\mylog.txt", "MINE TgInventoryManager::NonPersistAddDevice START - device ID %d", nDeviceId);

	Logger::Log("debug", "MINE TgInventoryManager::NonPersistAddDevice START\n");

	ATgPawn* ownerpawn = (ATgPawn*)InventoryManager->Owner;
	ownerpawn->SetDevice(nDeviceId);

	ATgDevice* beacondevice = ownerpawn->c_PIEInHandDevice;

	ownerpawn->r_EquipDeviceInfo[nEquipPoint].nDeviceId = nDeviceId;
	ownerpawn->r_EquipDeviceInfo[nEquipPoint].nDeviceInstanceId = 1;
	ownerpawn->r_EquipDeviceInfo[nEquipPoint].nQualityValueId = 1165;

	ownerpawn->m_EquippedDevices[nEquipPoint] = beacondevice;

	if (ownerpawn->PlayerReplicationInfo) {
		ATgRepInfo_Player* repinfoplayer = (ATgRepInfo_Player*)ownerpawn->PlayerReplicationInfo;

		repinfoplayer->r_EquipDeviceInfo[nEquipPoint].nDeviceId = nDeviceId;
		repinfoplayer->r_EquipDeviceInfo[nEquipPoint].nDeviceInstanceId = 1;
		repinfoplayer->r_EquipDeviceInfo[nEquipPoint].nQualityValueId = 1165;
	}

	// LogToFile("C:\\mylog.txt", "MINE TgInventoryManager::NonPersistAddDevice END");

	Logger::Log("debug", "MINE TgInventoryManager::NonPersistAddDevice END\n");
	return beacondevice;
}

