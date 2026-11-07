#pragma once

#include "src/pch.hpp"

#include "lib/nlohmann/json.hpp"

// Restore a match from a MISSION_PROGRESS_SNAPSHOT taken on another instance
// of the same map + mode (control server message INSTANCE_RESTORE). The state
// is kept pending and applied once, when this instance's mission timer is
// running; if it is already running it is applied on the next game tick.
// Carried over: mission time left, team points (ticket/koth), objective owner
// + capture progress + captured-once (ticket), the current rotation point
// (koth), raid wave + wave time left, boss / defended-NPC health (raid, pve,
// superagent; lowered once the bot pawn exists), payload checkpoints (lock +
// capture state) and robot track position (escort matinees), each player's
// location / rotation / health / power pool (PlayerState). Not carried:
// other actors, koth point owner / next-rotation countdown.
namespace InstanceRestore {

void SetPending(const nlohmann::json& progress);

void MaybeApply(ATgGame* Game);

// GetTickCount() when the last pending state was applied (the match state
// is in place, round-start cleanup done), 0 while one is still pending.
DWORD AppliedAt();

// koth: objective array position the next rotation pick must use (consumed
// once), or -1. Called from TgGame_PointRotation::CalcNextObjective.
int TakeRotationIndex();

}  // namespace InstanceRestore
