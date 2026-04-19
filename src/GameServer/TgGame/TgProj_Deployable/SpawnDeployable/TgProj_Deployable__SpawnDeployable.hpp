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
	static ATgDeployable* SpawnDeployableActor(ATgPawn* pawn, int deployableId, FVector vLocation, FVector vNormal);

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
};
