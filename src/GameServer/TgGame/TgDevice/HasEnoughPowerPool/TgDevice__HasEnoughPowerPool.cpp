#include "src/GameServer/TgGame/TgDevice/HasEnoughPowerPool/TgDevice__HasEnoughPowerPool.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgDevice__HasEnoughPowerPool::Call(ATgDevice* Device, void* edx, int FireModeNum) {
	bool originalresult = CallOriginal(Device, edx, FireModeNum);

	Logger::Log(GetLogChannel(), "HasEnoughPowerPool(device: 0x%p, firemodenum: %d) -> %d\n", Device, FireModeNum, originalresult);

	return originalresult;
}

