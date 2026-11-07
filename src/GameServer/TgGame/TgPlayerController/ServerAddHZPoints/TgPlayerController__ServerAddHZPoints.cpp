#include "src/GameServer/TgGame/TgPlayerController/ServerAddHZPoints/TgPlayerController__ServerAddHZPoints.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerAddHZPoints::Call(ATgPlayerController* Controller, void* edx, int nValue, int nCharId) {
	LogCallBegin();
	// Never let a client add Agenda Points - AP is earned in play
	// only (control server). The original native topped up via Hi-Rez's shop.
	Logger::Log("vendor", "[AP] ServerAddHZPoints value=%d char=%d refused\n", nValue, nCharId);
	LogCallEnd();
}
