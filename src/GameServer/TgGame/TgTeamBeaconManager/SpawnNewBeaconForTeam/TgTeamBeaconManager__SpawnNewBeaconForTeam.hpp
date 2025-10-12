#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgTeamBeaconManager__SpawnNewBeaconForTeam : public HookBase<
	void(__fastcall*)(ATgTeamBeaconManager*, void*),
	0x109EE6B0,
	TgTeamBeaconManager__SpawnNewBeaconForTeam> {
public:
	static void __fastcall* Call(ATgTeamBeaconManager* BeaconManager, void* edx);
	static inline void __fastcall* CallOriginal(ATgTeamBeaconManager* BeaconManager, void* edx) {
		m_original(BeaconManager, edx);
	}
};

