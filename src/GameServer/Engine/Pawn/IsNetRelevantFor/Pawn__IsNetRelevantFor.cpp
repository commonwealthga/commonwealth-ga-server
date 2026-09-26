#include "src/GameServer/Engine/Pawn/IsNetRelevantFor/Pawn__IsNetRelevantFor.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

// Return type is `int`, not `bool`: the native is a UE3 UBOOL (a DWORD), and a
// bool hook would only set AL on the pass-through, leaving EAX's upper bits as
// whatever the original left there. The engine reads the full return value.
int __fastcall Pawn__IsNetRelevantFor::Call(APawn* Pawn, void* edx, AActor* RealViewer,
                                            AActor* ViewTarget, float* SrcLocation) {
	// Only player-controlled TgPawns get the per-viewer stealth treatment. Their
	// bAlwaysRelevant is cleared at spawn (SpawnPlayerCharacter) so the engine
	// actually consults this relevance test for them — an always-relevant actor
	// skips it, which is why the gate previously had no effect. Everything else
	// (bots, deployables, …) uses the engine's own decision.
	ATgPawn* pawn = nullptr;
	if (Pawn != nullptr && ObjectClassCache::ClassNameContains((UObject*)Pawn, "TgPawn")) {
		AController* ctrl = Pawn->Controller;
		if (ctrl != nullptr && ObjectClassCache::ClassNameContains((UObject*)ctrl, "PlayerController"))
			pawn = (ATgPawn*)Pawn;
	}
	if (pawn == nullptr)
		return CallOriginal(Pawn, edx, RealViewer, ViewTarget, SrcLocation);

	// Emulated bAlwaysRelevant: a player pawn is relevant to every connection
	// unless its cloak is hidden from THIS viewer. Stealthed NPCs never reach here
	// (they aren't PlayerController-controlled), so their audio cues/rep survive.
	if (!pawn->r_bIsStealthed) return 1;

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
	if (viewer == nullptr) return 1;

	// Friendlies see the cloak; stealth only hides from enemies.
	if (!pawn->IsEnemy((AActor*)viewer)) return 1;

	// Revealed to this viewer => stay relevant. These are exactly the clauses of
	// ATgPawn::ShouldUpdateStealthedFor (the client's own body-hide predicate):
	//   - damage/scanner reveal scalar   (m_fMakeVisibleCurrent)
	//   - deployable-sensor alert bit    (r_nSensorAlertLevel)
	//   - the viewer's personal scanner  (Visual Scanner / Sensor Boost)
	if (pawn->m_fMakeVisibleCurrent != 0.0f) return 1;
	if (pawn->r_nSensorAlertLevel != 0) return 1;
	if (viewer->ScannerSeeStealthedPlayer(pawn)) return 1;

	// Cloaked enemy, undetected, unrevealed: not relevant to THIS connection.
	return 0;
}
