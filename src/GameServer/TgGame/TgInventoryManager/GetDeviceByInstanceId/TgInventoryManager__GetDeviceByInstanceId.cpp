#include "src/GameServer/TgGame/TgInventoryManager/GetDeviceByInstanceId/TgInventoryManager__GetDeviceByInstanceId.hpp"
#include "src/Utils/Logger/Logger.hpp"

ATgDevice* __fastcall TgInventoryManager__GetDeviceByInstanceId::Call(ATgInventoryManager* Manager, void* edx, int nDeviceInstanceId) {
	LogCallBegin();
	ATgDevice* result = CallOriginal(Manager, edx, nDeviceInstanceId);
	LogCallEnd();
	return result;
}
