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

// Spawn gate: may we create an exit beacon for this team right now?
//
//     r_Beacon == null  AND  nobody on the team holds the slot-11 device
//
// Strictly stronger than the `mgr->r_Beacon == nullptr` test the spawn paths
// used to run: r_Beacon is null for the WHOLE time a beacon is carried
// (PickUpDeployable -> DestroyIt -> UnRegisterBeacon clears it), so that test
// happily minted a duplicate mid-carry.
//
// This is the same predicate as the binary's `ShouldSpawnBeacon` native
// (0x109ee6c0), but computed directly rather than by calling it. Two reasons,
// both deliberate:
//   * that native is `CheckBeacon(false) == false`, and our callers run from
//     INSIDE CheckBeacon's frame (it dispatches SpawnNewBeaconForTeam through
//     vtable[0x374]) — calling it re-enters the state machine mid-update via
//     ProcessEvent;
//   * it also short-circuits to false on
//     `s_bUsingBeaconInventory && !s_HexItem`, the hex/Territory beacon
//     mechanic we don't implement.
// Decompile of the native is kept at
// `decompiled/TgGame/ATgTeamBeaconManager/ATgTeamBeaconManager__ShouldSpawnBeacon/`.
bool ShouldSpawnBeacon(ATgTeamBeaconManager* mgr);

// Does any player on this team hold the beacon pickup device? Mirrors the
// carrier scan inside CheckBeacon exactly (r_TaskForce->m_TeamPlayers ->
// PRI->Owner -> Controller->Pawn -> slot 11 + m_bIsBeaconPlacing), so our gate
// and the binary's respawn decision can never disagree.
bool TeamHasBeaconCarrier(ATgTeamBeaconManager* mgr);

// True when this pawn is holding the beacon pickup device — slot 11 occupied
// by a device with `m_bIsBeaconPlacing`. This is *exactly* what the binary's
// `TgPawn::IsCarryingBeacon` (0x109be2d0) tests, and therefore exactly what
// `CheckBeacon`'s carrier scan keys on. Read it directly rather than through
// the SDK bool-returning wrapper (bitfield ReturnValue is unreliable).
bool PawnHoldsBeaconDevice(class ATgPawn* Pawn);

// Enforce "one exit beacon per team". `RegisterBeacon` (0x109f1ed0) assigns
// `r_Beacon = pBeacon` UNCONDITIONALLY — no compare, no teardown of the
// outgoing beacon — so a second registration silently orphans the first. The
// orphan stays a live actor with collision and a pickup cylinder that nothing
// tracks: `mgr->r_Beacon` no longer points at it, `TgPawn::KillDeployables`
// skips deployable id 36 as a team resource, and `AdjustBeaconForwardSpawn`
// only ever inspects `mgr->r_Beacon`. Nothing reaps it. This does.
//
// Walks `GRI->m_Deployables` (NOT GObjObjects) for deployable id 36 belonging
// to this manager's taskforce and destroys everything that isn't
// `mgr->r_Beacon`. Returns the number destroyed.
//
// ⚠️ Call only AFTER the keeper is registered. `eventDestroyIt` →
// UC Destroyed → `UnRegisterBeacon` → `CheckBeacon(true)`, which will respawn
// at a factory if it finds no beacon; with `r_Beacon` already installed that
// branch is unreachable. Re-entrancy-guarded regardless.
int ReapOrphanBeacons(ATgTeamBeaconManager* mgr);

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

// `Actor.SetCollisionType(ECollisionType)` — SDK wrapper trips the
// FunctionFlags |= ~0x400 bug. This re-derives the actor's collision AND its
// primitive component's collision from the type (unlike SetCollision, which
// only touches the actor flags), so it's what actually disables a static-mesh
// wall (COLLIDE_NoCollision = 1). Single-param, no bitfield hazard.
void SetCollisionType(AActor* actor, unsigned char newCollisionType);

// `Actor.SetLocation(FVector) : bool` — teleports the actor AND re-inserts it
// into the collision octree (a raw Location write leaves the octree stale).
// SDK wrapper trips the FunctionFlags bug. Returns nothing here (fire-and-move).
void SetLocation(AActor* actor, const struct FVector& newLocation);

// Carrier-loss cleanup: strip the pickup device (slot 11) and re-trigger
// CheckBeacon so the team's beacon respawns at its factory. Used by the death
// path and by the team-change teleport.
//
// Gated on `PawnHoldsBeaconDevice(Pawn)` — NOT on `mgr->r_BeaconHolder == pri`,
// which is what it used to test and which was wrong. `r_BeaconHolder` is
// overloaded by the binary's CheckBeacon: it is the CARRIER in the PICKED_UP
// branch, but the DEPLOYER (`r_Beacon->r_DRI->r_InstigatorInfo`) in the tail
// that runs for every live beacon. So the moment anybody else deployed or
// picked up, the old gate stopped matching the player who was actually holding
// a device — and his slot 11 was never cleaned. That leak is what kept the
// carrier FX on, blocked his stealth, let him deploy a duplicate beacon, and
// (because CheckBeacon's carrier scan then believed he was still carrying)
// suppressed the team's beacon respawn entirely.
void DropCarriedBeacon(class ATgPawn* Pawn);

// Beacon handoff for a pawn LEAVING its team without dying (-changeteam /
// autobalance teleport). Runs DropCarriedBeacon (respawns the beacon for the
// OLD team if the pawn was carrying it), then — if the pawn instead deployed
// the team's world beacon — severs its personal ownership link (Instigator +
// DRI r_InstigatorInfo) while leaving the beacon with the taskforce. Call
// BEFORE flipping the team so the respawn lands on the correct team.
void ReleaseBeaconForTeamChange(class ATgPawn* Pawn);

}  // namespace BeaconSdk
