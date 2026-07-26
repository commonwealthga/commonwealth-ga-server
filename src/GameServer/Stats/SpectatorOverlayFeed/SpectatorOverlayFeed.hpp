#pragma once

#include "src/pch.hpp"

// SpectatorOverlayFeed — per-pawn rate-limited push of live health + active
// effect ids, for the spectator broadcast overlay (browser-source HUD reading
// from the control server's HTTP endpoint). Purely transient/read-only: no DB
// writes, no gameplay effect, safe to disable at any time by not calling
// MaybePushSnapshot.
namespace SpectatorOverlayFeed {

// Called from Actor__Tick for every actor, every frame. Cheap-bails
// (class-name substring check) for anything that isn't a real player's
// TgPawn_Character, then rate-limits per-pawn so the actual IPC send only
// happens a few times a second even though this is called every frame.
void MaybePushSnapshot(AActor* actor);

// Drop the per-pawn rate-limit entry for pawnId (ATgPawn::r_nPawnId). Call
// this when a pawn is torn down (NetConnection__Cleanup) so the rate-limit
// map doesn't grow by one entry per connection for the life of the process.
void ForgetPawn(int pawnId);

}  // namespace SpectatorOverlayFeed
