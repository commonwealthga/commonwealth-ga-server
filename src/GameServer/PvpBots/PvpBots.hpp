#pragma once

// Player-like PvP bots, step 1 primitive.
// Dress a freshly spawned class profile bot (567 medic / 679 robotics /
// 680 assault / 681 recon) as a player agent: class HoloSuit, class level-1
// melee + ranged, a donor behavior (the class bots have none) and a name.

#include <string>
#include "src/Config/DifficultyScalar.hpp"

class ATgPawn;
class ATgAIController;
class ATgRepInfo_TaskForce;
struct FVector;
struct FRotator;

namespace PvpBots {

// Returns false for an unknown class name (nothing applied). `foe` (optional)
// seeds the bot's threat list so it engages at once (dev spawns in front of a
// player; the class bots' donor behaviors otherwise idle until aggro).
bool Dress(ATgPawn* pawn, const std::string& pvp_class, ATgPawn* foe = nullptr);

// Game-thread tick: deferred sight kick on `foe` for dressed bots (+ log).
void Tick(float DeltaSeconds);

// SetWhatToDoNext intercept: for a dressed bot far from / out of sight of its
// foe, rewrite the action pick to "run at the foe". No-op for everything else.
void OverrideMovement(ATgAIController* aic, int* nMovementCode, int* nMoveDestination);

// Remember a dressed bot so it comes back after it dies (same body,
// class, task force and spawn spot; re-seeded on its foe's current pawn).
// Dropped when the map changes, its task force is gone or the foe leaves.
void RegisterRespawn(ATgPawn* bot, int botId, const std::string& pvpClass,
                     ATgRepInfo_TaskForce* tf, bool enemySide,
                     const FVector& loc, const FRotator& rot, ATgPawn* foe,
                     DifficultyScalar scalar);

// TargetInLOS fallback for a tracked bot aiming at its own foe
// (clear world trace eye -> foe eye/centre, <= 3000 uu). False otherwise.
bool FireSight(ATgAIController* aic);

// Per-bot skill pacing of the fire check (true = may fire now).
bool FireAllowed(ATgAIController* aic, bool sight);

// Bots owned by a task force instead of a requesting player.
// AddTeamBot spawns a dressed `cls` bot at a start of task force 1|2 and keeps
// it respawning; returns its slot (0 = refused). RemoveTeamBots marks every
// slot with `tag` (or just `slot` when > 0) to leave at its next death, or
// kills it at once with `now`; returns the count. ReportRoster sends
// TEAM_ROSTER (players + bots per task force) to the control server.
int AddTeamBot(int taskForce, const std::string& cls, const std::string& tag, std::string* why,
               float skill = 0.5f);
int RemoveTeamBots(const std::string& tag, int slot, bool now);
void ReportRoster(const std::string& result);
int DevSetHealthPct(int pct, int taskForce);  // dev only (admin dev-kill target pvpbot_hp)

} // namespace PvpBots
