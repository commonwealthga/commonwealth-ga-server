#pragma once

#include "src/pch.hpp"

#include "lib/nlohmann/json.hpp"

// Payload (TgGame_Escort) state that lives outside the objective fields: each
// TgMissionObjective_Escort drives a Kismet matinee (its generated
// TgSeqEvent_ObjectiveMatineeCommander -> SeqAct_Interp) that moves the
// escort robot along the track. Read for the progress feed, written back by
// InstanceRestore.
namespace EscortState {

// One entry per GRI objective, same order as the feed's "objectives":
// the linked matinee's Position in seconds, or -1 (not an escort objective /
// no matinee found).
nlohmann::json CollectMatinee(TArray<ATgMissionObjective*>& arr);

// Location + yaw of the first escort objective's attached actor (the robot),
// or null.
nlohmann::json CollectRobot(TArray<ATgMissionObjective*>& arr);

// Seeks every escort objective's matinee to the saved Position (same array
// order; a matinee shared by several segments is seeked once). Returns the
// number of matinees moved.
int ApplyMatinee(TArray<ATgMissionObjective*>& arr, const nlohmann::json& positions);

}  // namespace EscortState
