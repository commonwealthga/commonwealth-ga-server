#pragma once

#include "src/pch.hpp"

// Objective arrow trail on 1P_ missions. The client's comma key
// (ShowPathToObjective) routes over the map's path network; on the desert
// 1P_SD* maps every objective sits on a different NetworkID than the start, so
// the route always fails ("Nav Error", msg 31313) and mission maps have no
// Kismet NavIndicator. Every ~30 s per player (and at once when the active
// objective changes) the server sends ClientShowPathTo: to the objective when
// it is on the player's own network, else to the node of the player's network
// nearest the objective. Disable with -noobjectiveguide=1.
//
// "node of the player's network" = the last node of that network
// on the cheapest route to the objective over all nav points (same network =
// connected, other networks closer than 1200 u = jump/drop hop), not the
// straight-line nearest one, which on 1p_SDColony04 pointed players walking
// to the ramp down back to the spot above the boss. Used by the arrows, the
// comma redirect and the Kismet NavIndicator redirect.
//
// Comma key: the client routes to the active objective's
// GetLocationActor (Bot objective = r_ObjectiveBot) else the objective's own
// Location. For a viewer whose network differs from the boss's, ReplicateActor
// sends that viewer r_ObjectiveBot=None and Location=<edge node of the viewer's
// network> (server values untouched), so the client's own route succeeds. The
// HUD tag + health bar keep using r_ObjectiveBotInfo (boss PRI).
//
// Route check (seen on 1P_SDDweller02: "kept getting that old nav
// error"): before every push the server asks a hidden server-side
// TgNavRouteIndicator (the same actor + native SetRoute the client uses for
// ClientShowPathTo) whether the client's route will work. Sends the objective
// when it does (its nav point if the target is a pawn that may not be
// net-relevant), else the furthest nav point along the cross-network route the
// client can still route to, else nothing (no more "Nav Error" every 30 s).
// Route graph = real reach specs + any-distance jump/drop hops (long gaps
// penalised, no 1200 u cut-off). No push while jetpacking/falling (max 10 s);
// immediate push when the player reaches another path network. Kismet
// NavIndicator redirect uses the same check. Log channel objguide.
class ObjectiveGuide {
public:
	static void Tick(float DeltaSeconds);
	// Called from ActorChannel__ReplicateActor for every actor; swaps the
	// per-viewer values around CallOriginal. Returns true if it replicated.
	static bool ReplicateObjective(AActor* Actor, ATgPawn* Viewer, void* Channel, void* edx);
	// Kismet TgSeqAct_NavIndicator (all maps): destination the client can
	// route to from the controller's pawn, or nullptr to drop the call.
	static AActor* KismetNavTarget(AController* C, AActor* dest);
};
