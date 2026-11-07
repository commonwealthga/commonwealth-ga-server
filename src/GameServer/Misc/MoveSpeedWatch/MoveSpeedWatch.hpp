#pragma once

#include "src/pch.hpp"

// Server-side overspeed telemetry. Native ServerMove accepts ClientLoc, so
// this does not snap anyone — it only logs when accepted displacement vs the
// pawn's current GroundSpeed/AirSpeed cap looks too fast for too long.
//
// Log channel: "move_speed". Enable in control-server.json.
namespace MoveSpeedWatch {

// Call AFTER CallOriginal on a move RPC so pawn->Location is the accepted loc.
void Observe(APlayerController* pc, float timestamp);

// Flush a SUMMARY line (if we have samples) and drop per-controller state.
void OnControllerDestroyed(APlayerController* pc);

}  // namespace MoveSpeedWatch
