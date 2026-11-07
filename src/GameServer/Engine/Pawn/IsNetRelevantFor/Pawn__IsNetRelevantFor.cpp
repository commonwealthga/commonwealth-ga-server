#include "src/GameServer/Utils/PerfProbe/PerfProbe.hpp"
#include "src/GameServer/Engine/Pawn/IsNetRelevantFor/Pawn__IsNetRelevantFor.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/GameServer/Utils/ActorTickLod/ActorTickLod.hpp"

// Return type is `int`, not `bool`: the native is a UE3 UBOOL (a DWORD), and a
// bool hook would only set AL on the pass-through, leaving EAX's upper bits as
// whatever the original left there. The engine reads the full return value.
int __fastcall Pawn__IsNetRelevantFor::Call(APawn* Pawn, void* edx, AActor* RealViewer,
                                            AActor* ViewTarget, float* SrcLocation) {
	PerfProbe::Scope _perf(PerfProbe::SLOT_RELEVANT);  // p1100e
	PerfProbe::Count(PerfProbe::CTR_RELEVANT);
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
	if (pawn == nullptr) {
		// Hub / open world: far AI pawns are skipped before the engine's
		// line-of-sight trace (see ActorTickLod).
		if (Pawn != nullptr && SrcLocation != nullptr && ActorTickLod::Active() && !Pawn->bAlwaysRelevant &&
		    ActorTickLod::DistSq(Pawn->Location, SrcLocation) >
		        ActorTickLod::kFarDistance * ActorTickLod::kFarDistance)
			return 0;
		return CallOriginal(Pawn, edx, RealViewer, ViewTarget, SrcLocation);
	}

	// Hub / open world: other players' pawns are relevant within kPlayerDistance.
	// The viewer's own pawn is always relevant.
	if (SrcLocation != nullptr && ActorTickLod::Active() && (AActor*)pawn != ViewTarget &&
	    (pawn->Controller != RealViewer) &&
	    ActorTickLod::DistSq(pawn->Location, SrcLocation) >
	        ActorTickLod::kPlayerDistance * ActorTickLod::kPlayerDistance)
		return 0;

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
