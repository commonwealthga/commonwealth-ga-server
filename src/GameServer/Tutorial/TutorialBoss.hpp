#pragma once

#include "src/pch.hpp"

// Skylark_P Elite Assassin: the Elite Assassin cutscene ends but no assassin
// spawns (chapter dead end).
// The map has no spawn action for the boss. Its Kismet:
//   SeqEvent_Touch_26 (TriggerVolume_28, the holding box under the arena,
//   not human-only, no outputs) stores whoever touches it in SeqVar_Object_115;
//   the player's TriggerVolume_18 touch plays the boss cine (SeqAct_Interp_1,
//   37 s) -> SeqAct_Destroy (cine stand-ins) -> SeqAct_Teleport_1
//   Target = SeqVar_Object_115 -> Note_1 in the arena.
// The boss is the TgMissionObjective_Bot's bot, spawned in the holding box
// from TgGame PostBeginPlay (UnlockObjective(1)) - before Kismet registers
// its touch events, so the touch can be lost (the bot idles in the box and
// the teleport has no target); seen live too: touch ok, teleport not run.
// Fix, found structurally (SDK object names lose their _N suffix):
//  1. identify: a touch event (not human-only) whose Instigator variable is
//     the Target of a SeqAct_Teleport -> once Kismet is up, a living AI pawn
//     inside its volume is reported to the event (CheckTouchActivate); if the
//     filter refuses a bot, the variable is set directly.
//  2. safety net: when the human-only touch whose chain reaches that teleport
//     has fired (boss cine), check 2 s after the cine ends (seen live: the boss
//     was identified but the Kismet teleport after
//     the cine never ran -> logged as "boss post-cine"), then 3x 20 s apart;
//     a boss that is not
//     alive near the teleport destination is moved there, a missing/dead boss
//     (objective not done) is respawned through the objective's own spawn
//     (same bot id / spawn table) and moved there. Logged (channel
//     "tutorial") and reported to the control server (GAME_EVENT
//     subtype tutorial_boss, visible in the live journal).
namespace TutorialBoss {

void Tick(float DeltaSeconds);

}  // namespace TutorialBoss
