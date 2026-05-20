#include "src/GameServer/TgGame/TgPawn/TrackDamageTaken/TgPawn__TrackDamageTaken.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called on the VICTIM pawn when they take damage.
// Accumulates damage taken in r_Scores[STYPE_DAMAGETAKEN].
void __fastcall TgPawn__TrackDamageTaken::Call(ATgPawn* Pawn, void* edx, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDamage);

	if (nDamage > 0 && Pawn) {
		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
		if (PRI != nullptr) {
			PRI->r_Scores[3] += nDamage;  // STYPE_DAMAGETAKEN
			PRI->bNetDirty = 1;
		}

		// Stealth shimmer-on-damage. Bump server-side `m_fMakeVisibleCurrent`
		// (pawn+0xEFC) — the actual visibility scalar that the binary uses
		// for SetMeshScalarValue("MakeVisible", ...) on the mesh, and that
		// TgPlayerController::CheckStealthedCharacter reads on the viewer
		// to decide if this pawn is visible from their POV.
		//
		// Replication to clients is handled centrally in
		// TgPawn__TickMakeVisibleCalculation — each server tick it computes
		// the delta vs. last-replicated value, writes
		// `r_fMakeVisibleIncreased` accordingly (the rep delta field that
		// the client's NativeReplicatedEvent folds into its local copy).
		// That means a damage event here just needs to mutate
		// m_fMakeVisibleCurrent; the next tick picks up the delta.
		//
		// IsStealthActive equivalent: r_bIsStealthed bit set AND no active
		// disablers (jetpack bumps r_nStealthDisabled).
		if (Pawn->r_bIsStealthed && Pawn->r_nStealthDisabled == 0) {
			Pawn->m_fMakeVisibleCurrent += 0.5f;
			if (Pawn->m_fMakeVisibleCurrent > 1.0f) Pawn->m_fMakeVisibleCurrent = 1.0f;
		}
	}

	LogCallEnd();
}
