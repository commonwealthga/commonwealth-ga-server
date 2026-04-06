#include "src/GameServer/TgGame/TgPlayerController/ServerRequestBeaconNetworkHop/TgPlayerController__ServerRequestBeaconNetworkHop.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerRequestBeaconNetworkHop::Call(ATgPlayerController* Controller, void* edx, ATgStartPoint* pStartPoint) {
	LogCallBegin();
	CallOriginal(Controller, edx, pStartPoint);
	LogCallEnd();
}
