#pragma once

#include <string>

namespace TgPlayerActions::TopDownCmd {

// Toggle a "top-down" view for the player owning session_guid.
//
// First call (no entry stored for this guid):
//   - snapshot the pawn's Location, Rotation, Physics, collision/gravity flags,
//     plus the Controller's Rotation
//   - lift the pawn by lift_z world-units (0 = use default, see TopDown.cpp)
//   - set Physics=PHYS_None + bSimulateGravity=0 + bCollideWorld=bCollideActors=0
//     so the pawn freezes mid-air without falling or getting bumped
//   - set Controller->Rotation.Pitch to -16384 (UE3 rotator: pitch straight down)
//     and sync c_nCameraYawOffset / c_nCameraPitchOffset so the next
//     GetPlayerViewPoint paints the top-down view
//
// Second call (entry already stored): restore every field from the snapshot,
// clear the entry. Subsequent calls cycle again.
//
// If the pawn died/respawned between calls the snapshot's location is stale;
// the restore proceeds anyway — re-issuing -topdown on a fresh pawn will just
// snapshot the new state and apply top-down again.
void Execute(const std::string& session_guid, float lift_z);

} // namespace TgPlayerActions::TopDownCmd
