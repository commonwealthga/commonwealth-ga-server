#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__CanPlayerUseVolume : public HookBase<
	bool(__fastcall*)(ATgPlayerController*, void*, ATgOmegaVolume*),
	0x10963550,
	TgPlayerController__CanPlayerUseVolume> {
public:
	static bool __fastcall Call(ATgPlayerController* PlayerController, void* edx, ATgOmegaVolume* Volume);
	static inline bool __fastcall CallOriginal(ATgPlayerController* PlayerController, void* edx, ATgOmegaVolume* Volume) {
		return m_original(PlayerController, edx, Volume);
	};
};

