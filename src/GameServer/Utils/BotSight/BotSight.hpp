#pragma once

#include "src/pch.hpp"

// Bot-to-bot sight.
//
// The engine only raises the sight events between a player and an AI, so two
// AI-controlled pawns on hostile teams never notice each other. A bot factory
// whose map_object_config row `bot_sight_radius` is > 0 opts in: twice a second,
// each of its idle bots (no Enemy) that a player is near enough to watch looks
// for the closest bot from an opted-in factory on another task force within that radius,
// and gets it through the game's own SeePlayer event.
// The game's enemy check (GRI CheckIsEnemy) puts every AI pawn on one side in
// the open world, so IsHostilePair answers it for those opted-in pairs.
namespace BotSight {

void Tick(float DeltaSeconds);

// True when a and b (pawns or their controllers) are bots of opted-in
// factories on different task forces, as of the last scan.
bool IsHostilePair(AActor* a, AActor* b);

// True when ctrl is an opted-in bot whose current target (TgPawn.r_TargetActor)
// is a hostile opted-in bot inside its factory bot_sight_radius. Line traces
// between two AI pawns always fail, so TargetInLOS uses this as its fallback.
bool TargetInRadius(ATgAIController* ctrl);

}
