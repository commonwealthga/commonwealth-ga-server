#include "src/GameServer/TgGame/TgPawn/SetProperty/TgPawn__SetProperty.hpp"
// #include "src/Utils/Logger/Logger.hpp"   // re-add if you re-enable the diagnostic block below

// Pass-through wrap of TgPawn::SetProperty (binary @ 0x109bf420).
//
// Kept installed (rather than removed) so it's a one-line edit to instrument
// any future "is SetProperty being called for prop X with value Y" question.
// Originally added during the power-station-regen postmortem (2026-05-07) to
// confirm UC's ApplyToProperty actually reaches the binary's SetProperty for
// prop 243. Re-used same day to diagnose the scope/jetpack compounding bug
// (props 49 GROUND_SPEED / 70 AIR_SPEED) — apply/remove pairing was visible
// in the (rawBefore, NewValue, rawAfter) tuple per call.
//
// To re-enable diagnostic logging: uncomment the Logger include above and the
// block below, edit the propId filter / message to match what you're chasing.
void __fastcall TgPawn__SetProperty::Call(ATgPawn* Pawn, void* edx, int PropertyId, float NewValue) {
	// if (Pawn && (PropertyId == /*propId*/ || PropertyId == /*another*/)) {
	// 	float rawBefore = -99999.0f;
	// 	if (Pawn->s_Properties.Data) {
	// 		for (int j = 0; j < Pawn->s_Properties.Num(); ++j) {
	// 			UTgProperty* p = Pawn->s_Properties.Data[j];
	// 			if (p && p->m_nPropertyId == PropertyId) { rawBefore = p->m_fRaw; break; }
	// 		}
	// 	}
	// 	Logger::Log("effects",
	// 		"[SET-PROP] PRE  pawn=%p prop=%d NewValue=%.3f  rawBefore=%.3f\n",
	// 		(void*)Pawn, PropertyId, NewValue, rawBefore);
	// 	CallOriginal(Pawn, edx, PropertyId, NewValue);
	// 	float rawAfter = -99999.0f;
	// 	if (Pawn->s_Properties.Data) {
	// 		for (int j = 0; j < Pawn->s_Properties.Num(); ++j) {
	// 			UTgProperty* p = Pawn->s_Properties.Data[j];
	// 			if (p && p->m_nPropertyId == PropertyId) { rawAfter = p->m_fRaw; break; }
	// 		}
	// 	}
	// 	Logger::Log("effects",
	// 		"[SET-PROP] POST pawn=%p prop=%d  rawAfter=%.3f  (delta=%+.3f)\n",
	// 		(void*)Pawn, PropertyId, rawAfter, rawAfter - rawBefore);
	// 	return;
	// }

	CallOriginal(Pawn, edx, PropertyId, NewValue);
}
