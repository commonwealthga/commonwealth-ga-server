#include "src/GameServer/TgGame/TgPawn/SetDeploySensorDetectedStealthLightup/TgPawn__SetDeploySensorDetectedStealthLightup.hpp"
#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped server native, gated in TgDeploy_Sensor.uc AddPlayerToList on
// (nActionFlag & 4) && (nTriggerEventFlag & 1). NOTE: retail data has no
// deployable visibility config with action bit2 (asm_data_set_visibility_configs:
// sensors use configs 13/25 only), so this is expected to NEVER fire — the
// deploy-sensor reveal is r_nSensorAlertLevel replication read by the client's
// ShouldUpdateStealthedFor (+0x1378 gate). Hook kept as a reveal window opener
// in case modded/unusual configs do carry the bit.
void __fastcall TgPawn__SetDeploySensorDetectedStealthLightup::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);

	// Unconditional entry log: zero [sensor] lines across all captured sessions
	// so far — must distinguish "native never called" from "called but gated".
	Logger::Log("stealth", "[sensor] called pawn=%d stealthed=%d disabled=%d m=%.2f\n",
		Pawn ? Pawn->r_nPawnId : -1, Pawn ? (int)Pawn->r_bIsStealthed : -1,
		Pawn ? Pawn->r_nStealthDisabled : -1, Pawn ? Pawn->m_fMakeVisibleCurrent : -1.0f);

	if (Pawn && Pawn->r_bIsStealthed) {
		TgPawn__TickMakeVisibleCalculation::QueueRevealPulse(Pawn->r_nPawnId, 1.0f);
		Logger::Log("stealth", "[sensor] reveal queued pawn=%d\n", Pawn->r_nPawnId);
	}

	LogCallEnd();
}
