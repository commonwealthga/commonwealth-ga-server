#pragma once

#include "src/pch.hpp"

// Ping-scaled soft client authority for player movement.
//
// UE3's stock MAXPOSITIONERRORSQUARED (3.0 ≈ 1.7 UU) triggers position
// corrections almost immediately during jetpack flight. At 100–200 ms RTT the
// client predicts ahead of the server and gets snapped back every few ticks.
//
// This module widens the trust zone up to 200 ms ping: when the client's
// reported ClientLoc is within a ping-scaled threshold of the server pawn,
// we accept the client position (sync server pawn + send ACK instead of
// ClientAdjustPosition). Errors above a hard cap always correct.
namespace MovementTolerance {

struct Thresholds {
	float xy = 0.0f;
	float z = 0.0f;
};

// Ping in milliseconds, clamped to [0, 200].
float GetPingMs(APlayerController* pc);

// XY/Z soft thresholds for the given pawn and ping.
Thresholds ComputeThresholds(APawn* pawn, float pingMs);

// Squared-error check against separate horizontal (XY) and vertical (Z) caps.
bool IsWithinThreshold(const FVector& a, const FVector& b, const Thresholds& t);

// Always correct above this (teleport / wall-clip / speed-hack guard).
bool ExceedsHardCap(const FVector& a, const FVector& b);

// Remember the latest ClientLoc from a ServerMove RPC (used by
// TrySuppressPendingAdjustment when SendClientAdjustment fires later).
void RecordClientMove(APlayerController* pc, const FVector& clientLoc);

// After ServerMove CallOriginal: if ClientLoc is within the soft zone, snap
// the server pawn to the client and queue an ACK instead of a correction.
bool TrySoftAuthorizeAfterMove(APlayerController* pc, APawn* pawn, const FVector& clientLoc);

// Before SendClientAdjustment CallOriginal: convert a pending correction into
// an ACK when the stored ClientLoc is still within the soft zone.
bool TrySuppressPendingAdjustment(APlayerController* pc, APawn* pawn);

} // namespace MovementTolerance
