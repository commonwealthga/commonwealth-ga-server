#include "src/GameServer/TgGame/TgPawn/UpdateHealer/TgPawn__UpdateHealer.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the pawn being HEALED to track who healed them.
// m_LastHealer is used in TrackKill for healing assist credit.
void __fastcall TgPawn__UpdateHealer::Call(ATgPawn* Pawn, void* edx, ATgPawn* Healer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Healer);

	// Ensure healer tracking is populated even if the original is a stub.
	if (Healer != nullptr && Healer != Pawn) {
		Pawn->m_LastHealer = Healer;
	}

	LogCallEnd();
}
