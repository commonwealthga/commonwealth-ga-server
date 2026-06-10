#include "src/GameServer/TgGame/TgPawn/SetDeploySensorDetectedStealthLightup/TgPawn__SetDeploySensorDetectedStealthLightup.hpp"
#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"

// Stripped server native. Retail sensor configs never carry the gating action
// bit (TgDeploy_Sensor.uc AddPlayerToList: nActionFlag&4 && nTriggerEventFlag&1),
// so this normally never fires — the deploy-sensor reveal runs off
// r_nSensorAlertLevel in TickMakeVisibleCalculation. Kept as a reveal opener
// in case unusual configs do carry the bit.
void __fastcall TgPawn__SetDeploySensorDetectedStealthLightup::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);

	if (Pawn && Pawn->r_bIsStealthed) {
		TgPawn__TickMakeVisibleCalculation::QueueRevealPulse(Pawn->r_nPawnId, 1.0f);
	}

	LogCallEnd();
}
