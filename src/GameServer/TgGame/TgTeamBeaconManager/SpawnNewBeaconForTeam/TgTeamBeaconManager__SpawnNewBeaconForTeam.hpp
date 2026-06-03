#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgTeamBeaconManager__SpawnNewBeaconForTeam : public HookBase<
	void(__fastcall*)(ATgTeamBeaconManager*, void*),
	0x109ee6b0,
	TgTeamBeaconManager__SpawnNewBeaconForTeam> {
public:
	static void __fastcall Call(ATgTeamBeaconManager* mgr, void* edx);
};
