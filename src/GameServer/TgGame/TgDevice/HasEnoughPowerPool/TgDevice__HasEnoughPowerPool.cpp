#include "src/GameServer/TgGame/TgDevice/HasEnoughPowerPool/TgDevice__HasEnoughPowerPool.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgDevice__HasEnoughPowerPool::Call(ATgDevice* Device, void* edx, int FireModeNum) {
	bool originalresult = CallOriginal(Device, edx, FireModeNum);

	Logger::Log("debug", "[TgDevice__HasEnoughPowerPool::Call] %d\n", (int)originalresult);


	return true;
}

