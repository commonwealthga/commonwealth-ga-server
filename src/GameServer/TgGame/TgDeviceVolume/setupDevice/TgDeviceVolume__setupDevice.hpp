#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

namespace DomeVrHealPad {
	ATgDeviceVolume* GetRegisteredVolume();
	// True when fm is the registered VR heal pad's fire mode — gate for the
	// "healpad" diagnostic channel in the TgDeviceFire hooks.
	bool IsPadFireMode(UTgDeviceFire* fm);
	// True for the pad's kept heal effect group ids (recorded by the ArmCheck
	// prune). SetEffectRep forces bIsBuff for these — the volume instigator
	// fails IsEnemy, so UC's IsBuff() reps the heal as a debuff and the client
	// FX tint inverts (yellow self / green enemy).
	bool IsPadHealEffectGroup(int egId);
	// One-shot post-PostBeginPlay arm check, driven from GameEngine__Tick.
	// UC PostBeginPlay disarms the pad when the stripped setupDevice exec
	// thunk misreports our hook's return; this samples the volume's armed
	// state ~5s after registration and redoes the UC arming if needed.
	void TickArmCheck();
}

namespace HazardVolumes {
	// Volumes whose wired device deals damage (the 14 damaging rows in
	// device_volumes.md — acid/toxic/lava/fire/shock/crusher; the 2801/2805
	// protection pads are excluded). Registered by setupDevice at map load.
	bool IsDamagingDevice(int deviceId);
	// True when loc falls inside a registered hazard volume's brush bounds.
	bool ContainsLocation(const FVector& loc);
}

class TgDeviceVolume_setupDevice : public HookBase<
	bool(__fastcall*)(ATgDeviceVolume*, void*),
	0x109aeec0,
	TgDeviceVolume_setupDevice> {
public:
	static bool __fastcall Call(ATgDeviceVolume* Volume, void* edx);
	static inline bool __fastcall CallOriginal(ATgDeviceVolume* Volume, void* edx) {
		return m_original(Volume, edx);
	};
};

