#pragma once

// Cheap per-deployable_id classification helpers backed by static DB rows in
// `asm_data_set_deployables`. All results cached after first lookup — these
// fire on every spawn AND from per-SetProperty mirror code, so the cost has
// to be one map lookup after the first DB hit.
//
// Lives at TgGame/_deployable_classify rather than under any one hook folder
// because multiple unrelated hooks ask the same questions:
//   - SpawnDeployable      — gates collision/effect-manager bootstrap, GRI register
//   - SetProperty          — chooses which engine fields to mirror per prop
//   - InitializeDefaultProps — seeds bomb-specific engine fields at base time
//   - TgDeviceFire::Deploy — gates pickup/consume behaviour
// Keeping these classifiers in a shared module avoids any hook depending on
// the implementation file of another hook.

namespace DeployableClassify {

	// True iff `asm_data_set_deployables.show_countdown_timer_flag == 1`.
	// Timer bombs (Shatter / EMP / Fire / Venom / Graviton, plus the named
	// special "* AVA BOMB" / "* Bomb ADDITIONAL DAMAGE") — anything that runs
	// a fuse-then-explode lifecycle from UC's bomb state path
	// (`s_fActivationTime > 0` + no `s_fProximityRadius`, so Active.BeginState
	// schedules `SetTimer('StartFire')` rather than the mine LifeSpan path).
	// Cached per id.
	bool IsTimerBomb(int nDeployableId);

	// True iff `asm_data_set_deployables.health > 0`. HP=0 deployables
	// (timer bombs, mines, most grenades) are pre-explosion / pre-trigger and
	// must NOT be hitscan-targetable, must NOT take damage, must NOT spawn
	// a per-actor EffectManager (which would otherwise let enemy bomb damage
	// destroy adjacent mines). Stations / turrets / destructibles carry
	// positive HP and remain damageable. Cached per id.
	bool IsDestructible(int nDeployableId);

	// True iff ANY device-mode that deploys this deployable has
	// `require_los_flag = 0` — i.e. the deploy action doesn't need a line-of-
	// sight aim trace because the deployable spawns on the firing pawn rather
	// than at an aimed location. Semantic, data-driven discriminator for
	// "self-spawning" deployables: spawn position should be `pawn->Location`,
	// not the aim hit point; no cylinder lift; collision-fail is bypassed.
	//
	// As of `server.db` v20 this picks out exactly the dome shield (deployable
	// 22, device 2886, device_mode 3304). Any future self-spawning deployable
	// gets the same treatment automatically by setting its device-mode row's
	// `require_los_flag = 0` — no code change needed.
	//
	// Replaces the old mesh-name `LIKE 'DEV_ForceField_Dome%'` discriminator
	// (which was fragile against renames and didn't generalise beyond force
	// fields). Cached per id.
	bool DeploysOnSelf(int nDeployableId);

}  // namespace DeployableClassify
