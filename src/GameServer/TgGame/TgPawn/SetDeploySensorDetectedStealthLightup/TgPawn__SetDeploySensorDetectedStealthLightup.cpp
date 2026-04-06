#include "src/GameServer/TgGame/TgPawn/SetDeploySensorDetectedStealthLightup/TgPawn__SetDeploySensorDetectedStealthLightup.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__SetDeploySensorDetectedStealthLightup::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
