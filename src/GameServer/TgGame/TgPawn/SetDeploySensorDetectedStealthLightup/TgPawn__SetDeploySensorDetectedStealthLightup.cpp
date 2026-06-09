#include "src/GameServer/TgGame/TgPawn/SetDeploySensorDetectedStealthLightup/TgPawn__SetDeploySensorDetectedStealthLightup.hpp"
#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped server native. Called on a stealthed pawn that a deployed Sensor
// currently detects (TgDeploy_Sensor.uc: while in the sensor radius). Its job
// is the make-visible reveal — the server-side sibling of the client-local
// SetScannerDetectedStealthLightup. We open/refresh a reveal window which
// TickMakeVisibleCalculation streams to clients as a partial reveal.
void __fastcall TgPawn__SetDeploySensorDetectedStealthLightup::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);

	if (Pawn && Pawn->r_bIsStealthed) {
		TgPawn__TickMakeVisibleCalculation::QueueRevealPulse(Pawn->r_nPawnId, 1.0f);
		Logger::Log("stealth", "[sensor] reveal pawn=%d stealthed=%d disabled=%d\n",
			Pawn->r_nPawnId, (int)Pawn->r_bIsStealthed, Pawn->r_nStealthDisabled);
	}

	LogCallEnd();
}
