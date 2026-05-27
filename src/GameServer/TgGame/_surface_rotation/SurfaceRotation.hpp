#pragma once

#include "src/SDK/SdkHeaders.h"

// Pure math: build a UE3 FRotator that aligns a deployable's local +Z axis with
// a surface normal, with the local +X axis pointing along the player's facing
// projected onto the surface plane.
//
// Used by deployable placement (sticky bombs / mines that adhere to walls and
// ceilings) so the actor sits flush on the surface instead of standing upright.
//
// Coordinate convention (UE3 standard):
//   +X = forward,  +Y = right,  +Z = up
//
// Algorithm:
//   1. Z = normalize(surfaceNormal)
//   2. X' = playerFacing - (playerFacing · Z) * Z      // project onto surface plane
//   3. If |X'| < EPS (player looking straight into/out of surface), fall back
//      to world forward perpendicularised against Z; if THAT also degenerates
//      (surface normal is world ±X), fall back to world right.
//   4. X = normalize(X')
//   5. Y = Z × X
//   6. Convert basis to FRotator via UE3's FMatrix::Rotator formula
//      (UnMath.cpp:577; see implementation comments for the derivation).

namespace SurfaceRotation {

	// Returns a FRotator whose rotation matrix has +Z aligned with
	// `surfaceNormal` and +X aligned with `playerFacing` projected onto the
	// surface plane. Both inputs are world-space; only direction matters
	// (lengths ignored — we normalize internally).
	//
	// The surface normal of a true horizontal ground (≈ +Z) with a horizontal
	// player facing reduces this to the player's usual yaw-only rotation, so
	// it's safe to call unconditionally.
	FRotator FromSurfaceNormal(const FVector& surfaceNormal,
	                            const FVector& playerFacing);

}  // namespace SurfaceRotation
