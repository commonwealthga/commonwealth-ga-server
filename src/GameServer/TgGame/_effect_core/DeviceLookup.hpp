#pragma once

#include "src/pch.hpp"

// DeviceLookup — canonical device-instance lookup, replacing the invented
// `DeviceCategorySkill` side-index.
//
// The original game resolves a device's class-skill id from a device instance
// id by scanning the owner pawn's equipped devices: the intact native
// `ATgPawn::GetDeviceByInstanceId` (0x109be240) is a linear scan of
// `m_EquippedDevices[0x19]` (@ +0x82C) matching `r_nDeviceInstanceId` (@ +0x220).
// We do the same scan inline so the lookup is a pure read over engine state —
// no ProcessEvent round-trip, no stored map, nothing to keep in sync.
//
// Category is NOT a device field — it lives on the effect group
// (`UTgEffectGroup::m_nCategoryCode`); buff queries pass `nReqCategoryCode = -1`
// (wildcard) and filter by skill + device instance.
// See .planning/effect-buff-property-canonical.md §4 (Q3).
namespace DeviceLookup {

// The equipped-device array width (ATgPawn::m_EquippedDevices[0x19]).
inline constexpr int kEquippedDeviceSlots = 0x19;

inline ATgDevice* DeviceByInstanceId(ATgPawn* owner, int deviceInstId) {
	if (!owner || deviceInstId <= 0) return nullptr;
	for (int i = 0; i < kEquippedDeviceSlots; ++i) {
		ATgDevice* d = owner->m_EquippedDevices[i];
		if (d && d->r_nDeviceInstanceId == deviceInstId) return d;
	}
	return nullptr;
}

// Class-skill id (ATgDevice::m_nSkillId @ +0x2B0) for the device, or 0 if the
// instance id is not currently equipped on `owner`.
inline int SkillIdForDevice(ATgPawn* owner, int deviceInstId) {
	ATgDevice* d = DeviceByInstanceId(owner, deviceInstId);
	return d ? d->m_nSkillId : 0;
}

}  // namespace DeviceLookup
