#include "src/GameServer/TgGame/TgPawn/TrackDamageTaken/TgPawn__TrackDamageTaken.hpp"
#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"

// Called on the VICTIM pawn when they take damage (native stripped in the binary).
// Accumulates damage-taken score, and on a stealthed victim queues a ~1s reveal.
void __fastcall TgPawn__TrackDamageTaken::Call(ATgPawn* Pawn, void* edx, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDamage);

	if (nDamage > 0 && Pawn) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[3] += nDamage;  // STYPE_DAMAGETAKEN
			PRI->bNetDirty = 1;
		}

		// Reveal-on-damage: hold the stealth reveal for ~1s, then full stealth returns.
		if (Pawn->r_bIsStealthed) {
			TgPawn__TickMakeVisibleCalculation::QueueRevealPulse(Pawn->r_nPawnId, 1.0f);
		}
	}

	LogCallEnd();
}
