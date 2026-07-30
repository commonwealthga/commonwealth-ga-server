#pragma once

#include "src/pch.hpp"

// MissionProgressFeed -- rate-limited push of live mission-progress state
// (ticket counts, raid wave/boss health, KOTH/payload/breach objective
// capture progress) for the spectator broadcast overlay. Sibling to
// SpectatorOverlayFeed (per-pawn health) but per-instance, not per-pawn:
// there is exactly one ATgGame per match, so this pushes once per interval
// total rather than once per pawn.
namespace MissionProgressFeed {

// Called from Actor__Tick for every actor, every frame. Cheap-bails unless
// the actor IS the cached singleton ATgGame (Globals::GGameInfo), then
// rate-limits so the actual IPC send only happens ~1/sec. Mission mode is
// derived from the game's own class name (TgGame_Ticket/_Escort/_Defense/
// _PointRotation/_Mission) -- classes with no requested overlay treatment
// (City, OpenWorldPVE/PVP, CTF, DualCTF) are silently skipped.
void MaybePushSnapshot(AActor* actor);

}  // namespace MissionProgressFeed
