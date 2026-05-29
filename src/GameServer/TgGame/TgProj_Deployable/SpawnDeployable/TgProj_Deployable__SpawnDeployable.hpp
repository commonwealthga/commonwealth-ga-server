#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgProj_Deployable__SpawnDeployable : public HookBase<
	ATgDeployable*(__fastcall*)(ATgProj_Deployable*, void*, FVector, AActor*, FVector),
	0x10a19c40,
	TgProj_Deployable__SpawnDeployable> {
public:
	static ATgDeployable* __fastcall Call(ATgProj_Deployable* pThis, void* edx, FVector vLocation, AActor* TargetActor, FVector vNormal);
	static inline ATgDeployable* __fastcall CallOriginal(ATgProj_Deployable* pThis, void* edx, FVector vLocation, AActor* TargetActor, FVector vNormal) {
		return m_original(pThis, edx, vLocation, TargetActor, vNormal);
	};

	// Shared helper used by both SpawnDeployable (projectile path) and TgDeviceFire::Deploy.
	// `sourceDevice` / `sourceFireMode` are the ATgDevice + UTgDeviceFire that issued
	// the deploy. Stored on the spawned deployable as r_Owner / s_SpawnerDeviceMode so
	// downstream code (effect attribution, per-type limits, source-trace for morale
	// credit, etc.) can chase from the deployable back to the equipping device.
	// Either may be null if the caller can't resolve them.
	static ATgDeployable* SpawnDeployableActor(ATgPawn* pawn, int deployableId,
		FVector vLocation, FVector vNormal,
		ATgDevice* sourceDevice = nullptr, UTgDeviceFire* sourceFireMode = nullptr);

	// True iff the deployed actor is a TgDeploy_Beacon (or subclass). Used by
	// Deploy to decide whether to consume the spawning device from inventory.
	// Class-name prefix match — SDK StaticClass() is unreliable on server.
	static bool IsBeaconDeployable(ATgDeployable* Deployable);

	// Resolve the UC class full-name for a given deployable_id via the
	// asm_data_set_deployables.class_res_id column. Returns a literal C string
	// like "Class TgGame.TgDeploy_Beacon" suitable for ClassPreloader::GetClass.
	// Result is cached per deployable_id across calls. Unknown rows fall back
	// to "Class TgGame.TgDeployable".
	static const char* GetDeployableClassName(int nDeployableId);

	// Resolve the placement collision cylinder (radius, halfHeight) for a
	// deployable, matching what the client's UpdateDeployModeStatus does when
	// computing the ghost-preview footprint. Source of truth:
	// asm_data_set_assembly_meshes (joined via asm_data_set_deployables.asm_id)
	// — 171/185 deployables have real values there. The 14 rows with zeros are
	// mines/grenades/sweep-sensors which aren't aim-placed; they fall back to
	// the UC default on Class TgGame.TgDeployable.CollisionCylinder:
	// Height=10, Radius=12. Cached per deployable_id.
	static void GetDeployableCollisionCylinder(int nDeployableId, float* outRadius, float* outHalfHeight);

	// Resolve the spawn-Z lift HALF-height for ground-snap placement. Uses the
	// LEGACY convention (`asm_height * 0.5`, no scale), which is what every
	// pre-scale-fix lift call site assumed. The match is empirical: stations
	// deployed at the right visible height under the old formula. Cylinder
	// install (`GetDeployableCollisionCylinder`) and spawn placement diverged
	// when the OLD `* 0.5` was replaced with `* scale * scale_3d_z` — that
	// raised actor.Location.Z by a scale-dependent amount on flat ground while
	// the mesh's anchor position didn't move, so stations visibly floated.
	// Keeping the lift on the legacy formula restores ground-snap. Cached per
	// deployable_id.
	static void GetDeployableSpawnZLift(int nDeployableId, float* outLiftHalfHeight);

	// True if the deployable_id resolves to TgDeploy_ForceField (or a subclass).
	// Used to set the ffCheck flag on the placement trace so the trace respects
	// the solid force-field volume instead of passing through it.
	static bool IsForceFieldDeployableId(int nDeployableId);

	// Resolve `Time To Deploy (secs)` for a deployable from prop 279 on its
	// thrower device-mode (asm_data_set_devices_data_set_device_modes joined
	// to asm_data_set_device_mode_properties on prop_id=279). Stations/turrets
	// have explicit per-rank values (5–15s); bombs / mines / force fields
	// lack the prop and return 0 — callers must skip the
	// `r_fTimeToDeploySecs = ...` assignment in that case so UC's
	// Deploy.BeginState runs its `ExtractDeployTimeFromMyAnimation` fallback
	// (gives mines a real arming window instead of an instant deploy).
	// Cached per deployable_id.
	static float GetDeployTimeSecs(int nDeployableId);

	// Same as GetDeployTimeSecs but keyed by bot_id (for pet/turret pawns
	// spawned via TgDeviceFire::SpawnPet, not the deployable path). Joins the
	// same prop 279 row through `m.bot_id = ?`. Default 2.0s fallback when
	// missing — matches the existing SpawnPet fallback.
	static float GetPetDeployTimeSecs(int nBotId);

	// Scale a base deploy-time value by the deploying pawn's buff registry.
	// Both `GetDeployTimeSecs` and `GetPetDeployTimeSecs` do raw DB reads of
	// prop 279 ("Time To Deploy (secs)"), bypassing the buff system. Skills
	// targeting prop 391 ("Pet Deploy Time Modifier", BUFF_DEVICE expansion of
	// 279) — e.g. Repair Arm Speed (-15%) and Cyber Specialist (-20%) — never
	// reach the deploy path without this hook. Calls GetBuffedProperty with
	// BUFF_DEVICE context, which in turn calls ConvertPropToPropList(3, 279)
	// → {391}, picks up the player's stacked skill modifiers, and returns the
	// scaled time. Pass-through when the pawn has no relevant buff.
	//
	// `deviceInstId` filters the buff registry to entries scoped to the
	// deploying device (`r_nDeviceInstanceId`) plus wildcards. Skills register
	// wildcard so they always fold in. Other equipped weapons' deploy-time
	// rolled mods (if any) stay scoped to their own device.
	static float ApplyDeployTimeBuff(class ATgPawn* pawn, float baseSecs, int deviceInstId);

	// Prop 150 PERSIST_TIME on the THROWING device-mode (the player's item).
	// Returns 0 when the row is missing. Caller (TgDeviceFire::Deploy /
	// SpawnDeployableActor) applies this as a fallback / override on
	// Deployable->s_fPersistTime when InitializeDefaultProps' internal-device
	// seed didn't pick anything up. DB convention is inconsistent — stations
	// carry prop 150 on the deployable's internal device (handled by the
	// seed), force fields / mines / bombs carry it on the THROWER device-mode
	// (handled by this helper). Cached per device_mode_id.
	static float GetThrowerPersistTime(int device_mode_id);

	// Prop 354 TGPID_PET_LIFESPAN on the spawn-pet device-mode. Drone-spawning
	// modes carry 10-20s typical; turrets typically lack this row (live until
	// killed). Returns 0 when missing. Caller (SpawnPet) writes the result
	// directly to the spawned bot's LifeSpan. Cached per device_mode_id.
	static float GetPetLifeSpanSecs(int device_mode_id);

	// Scale a base pet-lifespan value by the deploying pawn's buff registry.
	// `GetPetLifeSpanSecs` reads prop 354 raw from the DB, bypassing the buff
	// system — so skills targeting prop 355 ("Pet Lifespan Modifier", the
	// BUFF_DEVICE expansion of 354 emitted by ConvertPropToPropList srcType=3
	// case 354) never reach the spawn path without this hook. Examples:
	// Drones skills 631/797/798 (+10/15/20%), Infiltration skill 837 (+30%).
	// Calls GetBuffedProperty with SKILL context — same call shape as
	// ApplyDeployTimeBuff. Pass-through when the pawn has no relevant buff.
	// `deviceInstId` filters the buff registry to entries scoped to the
	// drone-spawning device plus wildcards.
	static float ApplyPetLifeSpanBuff(class ATgPawn* pawn, float baseSecs, int deviceInstId);

	// Resolve TGPID_MAX_DEPLOYABLES_OUT (prop 154) for a fire-mode. Returns
	// the configured limit (1 for stations / force fields / sensors, 2-3 for
	// mines / grenades, larger for bombs), or 0 when no row exists (no limit
	// enforced). Cached per device_mode_id.
	static int GetMaxDeployablesOut(int device_mode_id);

	// Drops null + already-destroyed entries from `pawn->s_SelfDeployableList`
	// in place, preserving the relative order of the remaining entries. Without
	// this, the list grows unbounded (preallocated to 255 slots) and would
	// eventually overflow on a long-lived pawn that keeps deploying & losing
	// entries to natural destruction. Also called as the first step of
	// EnforceDeployableLimit so the count reflects only live deployables.
	static void CompactDeployableList(ATgPawn* pawn);

	// Enforce the per-fire-mode limit (prop 154) for a freshly-spawned
	// deployable `newDep`. Walks the GLOBAL `GRI.m_Deployables` (survives
	// pawn death + respawn — `pawn->s_SelfDeployableList` was per-pawn and
	// reset to empty on respawn, letting players bypass the limit by dying
	// between deployments). Matches by:
	//   - owner PRI identity (`dep->r_DRI->r_InstigatorInfo == pawn->GetPRI()`).
	//     The PRI lives on the PlayerController and persists across pawn
	//     deaths, so the player's deployables stay attributed to them even
	//     after respawn.
	//   - same `r_nDeployableId` (so different deployable types each get
	//     their own independent count — one medical station + one power
	//     station coexist; a 2nd of either kills the prior of that kind).
	//   - `dep != newDep` (the just-spawned actor is already in
	//     `gri->m_Deployables` because `RegisterDeployableInGRI` ran earlier
	//     in the spawn pipeline — skipping it prevents the limit logic from
	//     killing the very deployable that just triggered it).
	// When the live count would exceed the limit, calls
	// `eventDestroyIt(false)` on the oldest matching prior entries until the
	// (pre-existing + 1 new) total equals the limit. No-op when the firing
	// fire mode has no prop-154 row or the pawn has no PRI.
	static void EnforceDeployableLimit(ATgPawn* pawn, ATgDeployable* newDep, UTgDeviceFire* sourceFireMode);

	// Append `dep` to the global `GRI.m_Deployables` list (TgRepInfo_Game's
	// world-wide deployable registry, replicated to every client). UC's
	// TgBeaconFactory.uc:58 iterates this list, and the HUD's device-bar
	// healthbar pipeline likely walks it too — without it, any client-side
	// "show me all deployables" enumeration sees zero entries on our server.
	// The native that should populate it is stripped. Compacts (drops null
	// + already-destroyed entries) before appending so the list doesn't
	// grow unbounded across a long match.
	static void RegisterDeployableInGRI(ATgDeployable* dep);
};
