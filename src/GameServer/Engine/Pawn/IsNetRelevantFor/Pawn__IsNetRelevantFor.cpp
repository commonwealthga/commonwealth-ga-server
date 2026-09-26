#include "src/GameServer/Engine/Pawn/IsNetRelevantFor/Pawn__IsNetRelevantFor.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

// Return type is `int`, not `bool`: the native is a UE3 UBOOL (a DWORD), and a
// bool hook would only set AL on the pass-through, leaving EAX's upper bits as
// whatever the original left there. The engine reads the full return value.
int __fastcall Pawn__IsNetRelevantFor::Call(APawn* Pawn, void* edx, AActor* RealViewer,
                                            AActor* ViewTarget, float* SrcLocation) {
	const int result = CallOriginal(Pawn, edx, RealViewer, ViewTarget, SrcLocation);

	// Engine already says "not relevant" (cull distance, owner, base, …): nothing
	// to add, and we must not resurrect a culled actor.
	if (!result) return result;

	// Only TgPawns can be cloaked. Everything else passes straight through.
	if (Pawn == nullptr || !ObjectClassCache::ClassNameContains((UObject*)Pawn, "TgPawn"))
		return result;

	// Player-controlled pawns only. Stealthed NPCs stay relevant on purpose: they
	// still emit audio cues / drive behaviour replication that would be lost if we
	// de-replicated them.
	AController* ctrl = Pawn->Controller;
	if (ctrl == nullptr || !ObjectClassCache::ClassNameContains((UObject*)ctrl, "PlayerController"))
		return result;

	ATgPawn* pawn = (ATgPawn*)Pawn;
	if (!pawn->r_bIsStealthed) return result;

	// Resolve the viewing pawn: the connection's ViewTarget, else the viewer
	// controller's pawn. No viewer => can't prove it's an enemy => keep relevant.
	ATgPawn* viewer = nullptr;
	if (ViewTarget != nullptr && ObjectClassCache::ClassNameContains((UObject*)ViewTarget, "TgPawn")) {
		viewer = (ATgPawn*)ViewTarget;
	} else if (RealViewer != nullptr &&
	           ObjectClassCache::ClassNameContains((UObject*)RealViewer, "PlayerController")) {
		APawn* viewerPawn = ((APlayerController*)RealViewer)->Pawn;
		if (viewerPawn != nullptr && ObjectClassCache::ClassNameContains((UObject*)viewerPawn, "TgPawn"))
			viewer = (ATgPawn*)viewerPawn;
	}
	if (viewer == nullptr) return result;

	// Friendlies see the cloak; stealth only hides from enemies.
	if (!pawn->IsEnemy((AActor*)viewer)) return result;

	// Revealed to this viewer => stay relevant. These are exactly the clauses of
	// ATgPawn::ShouldUpdateStealthedFor (the client's own body-hide predicate):
	//   - damage/scanner reveal scalar   (m_fMakeVisibleCurrent)
	//   - deployable-sensor alert bit    (r_nSensorAlertLevel)
	//   - the viewer's personal scanner  (Visual Scanner / Sensor Boost) — the
	//     viewer's own r_ScannerSettings grant, otherwise the holder gets a
	//     pawn-less client and can't reveal anyone.
	if (pawn->m_fMakeVisibleCurrent != 0.0f) return result;
	if (pawn->r_nSensorAlertLevel != 0) return result;
	if (viewer->ScannerSeeStealthedPlayer(pawn)) return result;

	// Cloaked enemy, undetected, unrevealed: not relevant to THIS connection.
	return 0;
}
