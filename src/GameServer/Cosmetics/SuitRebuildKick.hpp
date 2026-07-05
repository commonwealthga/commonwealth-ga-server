#pragma once

#include "src/pch.hpp"

// ── Deferred owner-side rebuild kick after profile switch ────────────────────
//
// The client applies suit/helmet/hair meshes in native ApplyPawnSetup →
// FUN_109d1860 → GDataStore mesh-apply (0x109a6370 → 0x109a55b0). The apply is
// a SYNCHRONOUS StaticLoadObject by name with no retry: if the load returns
// null (which happens under -seekfreeloading when the same package is still
// queued in the async loader from the PREVIOUS switch's UpdateCharacterAssetRefs
// preload), it logs "Unable to load custom character suit mesh %d" and skips —
// the component keeps the old mesh while dyes (same ApplyPawnSetup pass, no
// asset load needed) apply fine. Observed ~1 in 50 rapid switches.
//
// Recovery is inherent in the same code: the apply short-circuits when the
// component's cached asm id (+0x4e8) equals the target row id, and reloads
// when they differ. So ANY later ApplyPawnSetup heals a failed apply and
// visually no-ops on a successful one.
//
// Trigger: r_nBodyMeshAsmId repnotify (TgPawn.uc:5789) runs ApplyPawnSetup +
// WaitForInventoryThenDoPostPawnSetup on every client, ungated by role. For
// custom characters the VALUE is ignored (GetBodyMeshId returns the suit id,
// TgPawn_Character.uc:369). The live rep list (Actor__GetOptimizedRepList, V1)
// delta-replicates it via NEQ against per-channel 'recent' state — so the kick
// writes a PERSISTENT alternating sentinel (-1↔-2) rather than a transient
// blip; a one-tick flip+restore coalesced to nothing on observer channels that
// skipped the frame (out/logs/278). Server-side, FireSockets reads the field
// for fire-socket lookups: player pawns hold 0 (invalid) there anyway, so the
// sentinel takes the same null-model fallback as steady state.
namespace SuitRebuildKick {

// Schedule a rebuild kick ~2s after a profile switch. Re-scheduling for the
// same pawn restarts the timer (rapid switches coalesce into one kick).
void Schedule(ATgPawn* Pawn);

// Drive pending kicks. Call once per server frame (GameEngine::Tick).
void Tick(float DeltaSeconds);

}  // namespace SuitRebuildKick
