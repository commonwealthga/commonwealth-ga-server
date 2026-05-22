#pragma once

#include "src/pch.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"

// OriginResolver — canonical resolution of the device that produced an effect,
// replacing the invented `DeployableOriginRegistry` side-maps.
// See .planning/effect-buff-property-canonical.md §4 (Q2).
//
// Two cases:
//   * Player / pet fired: UC `TgEffectGroup::InitInstance` populates
//     `m_nSourceDeviceInstId` / `m_nSourceDeviceSkillId` (@ +0x13C / +0x138)
//     from `Impact.DeviceModeReference`. When present (non-zero), they are
//     authoritative — use them directly.
//   * Deployable fired (e.g. Medical Station heal): the Impact carries no
//     `DeviceModeReference`, so `InitInstance` leaves the source ids at 0. The
//     source device is recovered WITHOUT a side-map by walking the deployable
//     the group's instigator points at:
//         m_Instigator (ATgDeployable)
//           -> s_SpawnerDeviceMode (UTgDeviceFire, +0x2A8)
//             -> m_Owner (ATgDevice, +0x3C)
//               -> { r_nDeviceInstanceId (+0x220), m_nSkillId (+0x2B0) }
//     This is the same chain the intact `ATgDeployable::GetSpawnerDeviceInstanceId`
//     / `GetSpawnerDeviceSkillId` natives walk; inlined here to stay independent
//     of the ProcessEvent wrappers.
namespace OriginResolver {

struct DeviceOrigin {
	int instId;   // ATgDevice::r_nDeviceInstanceId, or 0 if unresolved
	int skillId;  // ATgDevice::m_nSkillId, or 0 if unresolved
};

inline DeviceOrigin Resolve(UTgEffectGroup* eg) {
	DeviceOrigin out{0, 0};
	if (!eg) return out;

	// Player / pet path — InitInstance already set these from the Impact's
	// DeviceModeReference. Authoritative when present.
	if (eg->m_nSourceDeviceInstId != 0) {
		out.instId  = eg->m_nSourceDeviceInstId;
		out.skillId = eg->m_nSourceDeviceSkillId;
		return out;
	}

	// Deployable path — recover the spawner device from the deployable itself.
	AActor* inst = eg->m_Instigator;
	if (!inst || !ObjectClassCache::ClassNameContains(inst, "Deployable")) return out;

	UTgDeviceFire* fireMode = static_cast<ATgDeployable*>(inst)->s_SpawnerDeviceMode;
	if (!fireMode) return out;

	// fireMode->m_Owner is the spawning device (ATgDevice). Guard with a
	// class-name check (no IsA) before reading device fields — mirrors the
	// canonical Cast_TgDevice in GetSpawnerDeviceInstanceId.
	// TODO(Phase 3 verify): confirm deployable-spawning device actor class
	// names all carry the "Device" substring; tighten the needle if a
	// subclass (e.g. a weapon-class spawner) is found that does not.
	AActor* ownerActor = fireMode->m_Owner;
	if (!ownerActor || !ObjectClassCache::ClassNameContains(ownerActor, "Device")) return out;

	ATgDevice* dev = static_cast<ATgDevice*>(ownerActor);
	out.instId  = dev->r_nDeviceInstanceId;
	out.skillId = dev->m_nSkillId;
	return out;
}

}  // namespace OriginResolver
