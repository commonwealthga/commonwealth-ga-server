#include "src/GameServer/TgGame/TgGame/SpawnBot/TgGame__SpawnBot.hpp"
#include "src/Utils/Logger/Logger.hpp"

APawn* __fastcall TgGame__SpawnBot::Call(ATgGame* Game, void* edx, FName sName, FVector vLocation, FRotator rRotation, ATgBotFactory* pFactory, bool bIgnoreCollision, ATgPawn* pOwnerPawn, bool bIsDecoy, UTgDeviceFire* deviceFire, float fDeploySecs) {
	Logger::Log("debug", "TgGame__SpawnBot::Call");
	return CallOriginal(Game, edx, sName, vLocation, rRotation, pFactory, bIgnoreCollision, pOwnerPawn, bIsDecoy, deviceFire, fDeploySecs);
}


