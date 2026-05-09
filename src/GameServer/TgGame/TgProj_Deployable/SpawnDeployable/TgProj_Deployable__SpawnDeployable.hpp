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

	// True if the deployable_id resolves to TgDeploy_ForceField (or a subclass).
	// Used to set the ffCheck flag on the placement trace so the trace respects
	// the solid force-field volume instead of passing through it.
	static bool IsForceFieldDeployableId(int nDeployableId);

	// True if the deployable is a "timer bomb" — explodes after a fixed delay
	// when the player deploys it (EMP Bomb, Shatter Bomb, Fire Bomb, etc).
	// Discriminator: asm_data_set_deployables.show_countdown_timer_flag=1
	// (deliberately NOT deployable_type_value_id=666 — that value covers
	// everything from mines to sensors to force fields).
	// Cached per deployable_id.  Mines/grenades/stations return false.
	static bool IsTimerBombDeployableId(int nDeployableId);

	// Resolve timer-bomb activation time + damage radius from the device's
	// mode properties (TGPID_REMOTE_ACTIVATION_TIME=7, TGPID_DAMAGE_RADIUS=6).
	// Damage radius is stored in game-world feet; we return raw UU so callers
	// can write directly to s_fProximityRadius / r_fClientProximityRadius
	// (×16 conversion baked in here to match what ATgDeployable::ApplyProperty
	// would do for prop 8 on proximity mines).
	// Falls back to (3.0s, 320uu) when the DB lookup misses.
	// Cached per deployable_id.
	static void GetTimerBombParams(int nDeployableId,
		float* outActivationSecs, float* outDamageRadiusUU);

	// Resolve `Time To Deploy (secs)` for a deployable from prop 279 on its
	// thrower device-mode (asm_data_set_devices_data_set_device_modes joined
	// to asm_data_set_device_mode_properties on prop_id=279). Stations/turrets
	// have explicit per-rank values (5–15s); bombs / mines / force fields lack
	// the prop and fall back to 0.1s so they "deploy" effectively instantly.
	// Without this, every deployable inherits m_fDefaultDeployAnimLength via
	// UC's Deploy.BeginState fallback — close to identical across types.
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
	static float ApplyDeployTimeBuff(class ATgPawn* pawn, float baseSecs);

	// True iff asm_data_set_deployables.health > 0 for this deployable_id.
	// HP-zero deployables (Shatter Bomb, EMP Bomb, etc) are pre-explosion
	// timer bombs and most mines — they should NOT take damage and should
	// NOT be hitscan-targetable by enemies. Stations/turrets/destructibles
	// all carry positive HP and remain damageable. Cached per deployable_id.
	static bool IsDestructibleDeployableId(int nDeployableId);

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

	// Enforce the per-fire-mode limit (prop 154) before adding a new
	// deployable. Counts existing entries in `pawn->s_SelfDeployableList`
	// whose `r_Owner == sourceDevice` (the "same device" rule that lets a
	// player keep e.g. one medical station AND one power station, but not
	// two of either). When count >= limit, calls `eventDestroyIt(false)` on
	// the oldest matching entries until count < limit. The destroyed entries
	// stay in the list — they're filtered out next time CompactDeployableList
	// runs (next spawn) or wiped wholesale by KillDeployables on owner death.
	// No-op when sourceDevice is null or limit <= 0 (unlimited).
	static void EnforceDeployableLimit(ATgPawn* pawn, ATgDevice* sourceDevice, UTgDeviceFire* sourceFireMode);

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
