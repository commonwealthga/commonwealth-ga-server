#include "src/GameServer/TgGame/TgDevice/HasMinimumPowerPool/TgDevice__HasMinimumPowerPool.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgDevice__HasMinimumPowerPool::Call(ATgDevice* Device, void* edx, int FireModeNum) {
	bool originalresult = CallOriginal(Device, edx, FireModeNum);

	Logger::Log("debug", "[TgDevice__HasMinimumPowerPool::Call] %d\n", (int)originalresult);


	return true;
}

