#pragma once

#include "src/pch.hpp"

// Safe wrappers for beacon-system natives whose SDK-generated Parms structs
// hit the multi-`unsigned long :1` bitfield packing bug (see
// `memory/reference_sdk_bitfield_params_bug.md`):
//
// SDK auto-generates Parms with `unsigned long bX:1` at explicit 4-byte
// offsets. C++ default rules pack all such bitfields into ONE 4-byte storage
// unit, so the struct is smaller than UC expects AND UC reads/writes
// subsequent bool params from offsets the C++ struct doesn't even cover —
// stack corruption + lost return values.
//
// We bypass the SDK wrappers by emitting plain `uint32_t` per param structs
// that match UC's expected layout exactly, then ProcessEvent directly.
namespace BeaconSdk {

// `bool RegisterBeacon(TgDeploy_Beacon* pBeacon, optional bool bDeployed = true)`
bool RegisterBeacon(ATgTeamBeaconManager* mgr, ATgDeploy_Beacon* pBeacon, bool bDeployed);

// `bool CheckBeacon(optional bool bAttemptRespawn = true)`
bool CheckBeacon(ATgTeamBeaconManager* mgr, bool bAttemptRespawn);

// `void PopulateBeaconFactoryList()` — no params, safe to call via SDK,
// included here for one-stop shopping.
void PopulateBeaconFactoryList(ATgTeamBeaconManager* mgr);

// `TgRepInfo_TaskForce* GetTaskForce(int nTaskForceNum, optional bool bCreate)`
ATgRepInfo_TaskForce* GetTaskForce(ATgRepInfo_Game* GRI, int nTaskForceNum, bool bCreate);

// Walk GObjects for a TgTeamBeaconManager whose r_TaskForce matches the
// given team number. Use this instead of GRI->GetTaskForce(num, false) for
// timings where the GRI's TF lookup hasn't been populated yet (e.g. factory
// auto-spawn at world init).
ATgTeamBeaconManager* FindBeaconManagerByTaskForce(int taskforceNum);

// `Actor.SetCollision(bColActors, bBlockActors, bIgnoreEncroachers)` —
// SDK wrapper is hit by the multi-bitfield Parms bug and silently corrupts
// later bools. Manual dword-per-param ProcessEvent.
void SetCollision(AActor* actor, bool bColActors, bool bBlockActors, bool bIgnoreEncroachers);

// Carrier-loss cleanup. For each team manager whose `r_BeaconHolder` matches
// this pawn's PRI, strip the pickup device (slot 11) and re-trigger CheckBeacon
// — the native walks all PRIs, finds no IsCarryingBeacon holder, and respawns
// at the team's original-priority factory. Safe for any pawn (bails when no
// manager matches). Used by the death path and by the team-change teleport.
void DropCarriedBeacon(class ATgPawn* Pawn);

// Beacon handoff for a pawn LEAVING its team without dying (-changeteam /
// autobalance teleport). Runs DropCarriedBeacon (respawns the beacon for the
// OLD team if the pawn was carrying it), then — if the pawn instead deployed
// the team's world beacon — severs its personal ownership link (Instigator +
// DRI r_InstigatorInfo) while leaving the beacon with the taskforce. Call
// BEFORE flipping the team so the respawn lands on the correct team.
void ReleaseBeaconForTeamChange(class ATgPawn* Pawn);

}  // namespace BeaconSdk
