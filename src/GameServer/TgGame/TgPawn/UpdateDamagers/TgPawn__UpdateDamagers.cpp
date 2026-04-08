#include "src/GameServer/TgGame/TgPawn/UpdateDamagers/TgPawn__UpdateDamagers.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the VICTIM pawn when they take damage, to track recent damagers.
// Updates m_LastDamager/m_SecondToLastDamager for assist detection in TrackKill.
// Also calls the original which may do additional tracking.
void __fastcall TgPawn__UpdateDamagers::Call(ATgPawn* Pawn, void* edx, ATgPawn* Damager) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Damager);

	// Ensure damager tracking is populated even if the original is a stub.
	// Shift current LastDamager to SecondToLast if it's a different damager.
	if (Damager != nullptr && Damager != Pawn) {
		if (Pawn->m_LastDamager != Damager) {
			Pawn->m_SecondToLastDamager = Pawn->m_LastDamager;
		}
		Pawn->m_LastDamager = Damager;
	}

	LogCallEnd();
}
