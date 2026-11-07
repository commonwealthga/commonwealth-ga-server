#pragma once

#include "src/pch.hpp"
#include "lib/nlohmann/json.hpp"
#include <string>

// Ground steering for server-spawned groups that walk in
// open terrain (world groups with "engage"). Generic primitive: callers decide
// where a member should go; this keeps it on walkable ground (no cliffs, map
// edges or kill-Z, no walls/steep rock), walks around obstacles, detects
// "no progress" and reports targets it cannot reach. Cheap: a few world traces
// per member every 0.5 s while it moves.
namespace BossMove {

// Go = straight to the goal; Detour = around something; Hold = no safe step yet (stay);
// Blocked = chase gave up (mark the target unreachable); AtLimit = at the limit circle;
// Teleport = walking home is hopeless (put it home).
enum class Step { Go, Detour, Hold, Blocked, AtLimit, Teleport };

// Spawn/control keys (all optional): limit_r + limit_x/limit_y (never steer a
// chase further than limit_r from that 2D point), max_drop, max_rise (uu per
// 250 uu step), stuck_s (no progress -> detour / blocked), evade (bool: members
// hit only by targets they cannot reach take no damage and walk home).
void Configure(const std::string& tag, const nlohmann::json& j);
void Forget(const std::string& tag);
void ForgetPawn(ATgPawn* pawn);
bool EvadeEnabled(const std::string& tag);
// "engage_bots": true = hostile bots count as targets too (tests / faction fights; default players only).
bool TargetsBots(const std::string& tag);

// Where `pawn` should run next on its way to `goal`. chase = following a target
// (Blocked after repeated no-progress / no safe step; AtLimit at the limit
// circle); home walks get Teleport after repeated no-progress.
Step Steer(const std::string& tag, ATgPawn* pawn, const FVector& goal, bool chase, float now, FVector* out);
// Targets a member could not reach, remembered for a while (per tag).
void MarkUnreachable(const std::string& tag, APawn* foe, float now, float forS);
bool IsUnreachable(const std::string& tag, APawn* foe, float now);
// Turn to face a target (focus + desired yaw).
void Face(ATgPawn* pawn, APawn* foe);
// Evade bookkeeping (status only; the caller applies the protection).
void SetEvading(const std::string& tag, bool on, float now);
bool Evading(const std::string& tag);
// Diagnostics for group-status (per member + per tag).
void Status(const std::string& tag, ATgPawn* pawn, nlohmann::json& mj);
void TagStatus(const std::string& tag, nlohmann::json& msg);
// AI drive cost (microseconds of one DriveTick), for status.
void AddCost(const std::string& tag, double us);

} // namespace BossMove
