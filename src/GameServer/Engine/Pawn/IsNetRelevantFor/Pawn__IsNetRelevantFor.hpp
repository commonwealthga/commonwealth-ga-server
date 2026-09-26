#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// APawn::IsNetRelevantFor — engine native at 0x10C283A0, the per-(connection,
// actor) relevance test driven by UWorld::ServerTickClients @ 0x10C2B790 (and
// UWorld::Tick). Signature from the decompile:
//   bool(APawn* this, AActor* RealViewer, AActor* ViewTarget, FVector* SrcLocation)
// It has NO UnrealScript binding, so it carries no symbol and does not appear in
// names.txt / the SDK / by-name Ghidra search — it is reached only through the
// actor vtable slot (see ActorChannel__ReplicateActor / UWorld__ServerTickClients
// notes).
//
// Use: per-observer de-replication of stealthed players. The engine already hides
// the BODY client-side (ShouldUpdateStealthedFor), but the pawn actor still reaches
// every client, which leaks the jetpack trail / thrust FX and gives aimbots a
// target. Returning false here keeps the actor off that ONE connection entirely;
// the engine reopens it the moment the wearer becomes relevant again (reveal over,
// sensor alert, or viewer becomes a teammate).
class Pawn__IsNetRelevantFor : public HookBase<
	int(__fastcall*)(APawn*, void*, AActor*, AActor*, float*),
	0x10C283A0,
	Pawn__IsNetRelevantFor> {
public:
	static int __fastcall Call(APawn* Pawn, void* edx, AActor* RealViewer, AActor* ViewTarget, float* SrcLocation);
	static inline int __fastcall CallOriginal(APawn* Pawn, void* edx, AActor* RealViewer, AActor* ViewTarget, float* SrcLocation) {
		return m_original(Pawn, edx, RealViewer, ViewTarget, SrcLocation);
	}
};
