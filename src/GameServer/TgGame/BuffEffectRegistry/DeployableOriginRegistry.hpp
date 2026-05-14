#pragma once

// DeployableOriginRegistry — tracks the player's deploy device for each
// spawned deployable, so the deployable's fire-time buff queries can use
// the deploying-device's instId and skillId instead of the deployable's
// internal device's values.
//
// Why this is needed:
//   When a medical station fires its heal effect, UC's TgEffectGroup::
//   InitInstance sets `clone->m_nSourceDeviceInstId` from the firing
//   device's r_nDeviceInstanceId. For a station, that's the STATION's
//   internal heal device — NOT the player's deploy device. So:
//
//   - Rolled mods on the deploy device (registered in player's
//     m_EffectBuffInfo with `nReqDeviceInstId = playerDeployDevInstId`)
//     don't match queries that pass the station's internal devInstId.
//   - Multi-row skills (registered with `nReqSkillId > 0` keyed to a
//     device class) don't match because LookupByInstanceId walks the
//     player's Inventory::s_equipped — the station's internal device
//     isn't there.
//
//   Result: only universal (skillId=0, devInst=0) skill rows reach the
//   heal-output formula. Confirmed empirically: 153 base × 1.20
//   (Super Engineer's universal row) ≈ 184/tick instead of the expected
//   ~470/tick when all mods/skills apply.
//
// Fix: at deploy time, register `(deployable → spawnDevInstId,
// spawnDevSkillId)`. At clone time in CloneEffectGroup [EFFECT-BUFF]
// (and [DAMAGE-BUFF] for damage-dealing deployables like sentry mines),
// when the instigator is a deployable, look up its origin and override
// the buff query's instId+skillId.
class ATgDeployable;
class UTgDeviceFire;
class UTgEffectGroup;

namespace DeployableOriginRegistry {
	struct Origin {
		int spawnDeviceInstId;   // player's deploy-device r_nDeviceInstanceId
		int spawnDeviceSkillId;  // player's deploy-device m_nSkillId
	};

	// Register a deployable's spawning context. Called once at deploy time
	// from SpawnDeployableActor. `spawnDevInstId == 0` is acceptable when
	// no source device is available (server-spawned beacon, etc.) — falls
	// back to wildcard at query time.
	void Register(ATgDeployable* deployable, int spawnDevInstId, int spawnDevSkillId);

	// Look up. Returns {0, 0} when not registered.
	Origin Lookup(ATgDeployable* deployable);

	// Remove the entry when the deployable is destroyed. Optional cleanup —
	// the map grows otherwise but each entry is small. Call from a destroy
	// hook if one exists.
	void Unregister(ATgDeployable* deployable);

	// Effect-group-template → owning-fire-mode map.
	//
	// Used to recover the firing fire-mode from a cloned effect group when
	// UC's `InitInstance` left `clone->m_nSourceDeviceInstId == 0` (e.g.,
	// deployable-fired heals whose Impact has no DeviceModeReference).
	//
	// Populated by `TgDeviceFire::GetEffectGroup` when the lazy
	// s_EffectGroupList is constructed — each template eg pointer is
	// associated with the fire-mode that owns it. Templates are created
	// once per fire-mode lifetime and reused, so the map is set-once and
	// never races between simultaneous deployers.
	void NoteTemplateOwner(::UTgEffectGroup* templateEg, ::UTgDeviceFire* fireMode);
	::UTgDeviceFire* GetTemplateOwner(::UTgEffectGroup* templateEg);

	// Cloned effect group → deploy origin map.
	//
	// Why this exists separately from m_nSourceDeviceInstId on the clone:
	// UC's `TgEffectGroup::InitInstance` runs AFTER `CloneEffectGroup` and
	// unconditionally CLEARS m_nSourceDeviceInstId / m_nSourceDeviceSkillId
	// to 0 before only setting them if `Impact.DeviceModeReference != none`.
	// For deployable-fired heals whose Impact has no DeviceModeReference (the
	// common medical-station case), the engine fields stay 0 even if we
	// pre-populate them at clone time. This side-map preserves the origin
	// across InitInstance's clear so CheckEffectBuffModifier can recover it.
	void RegisterClone(::UTgEffectGroup* clone, int spawnDevInstId, int spawnDevSkillId);
	Origin LookupClone(::UTgEffectGroup* clone);
	void UnregisterClone(::UTgEffectGroup* clone);
}
