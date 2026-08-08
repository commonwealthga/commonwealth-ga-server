#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// ATgOmegaVolume::Used — reached server-side via
// TgPlayerController.ServerUse -> ServerPerformedUseAction ->
// P.r_CurrentOmegaVolume.Used(self). The retail body is client-only work
// (it opens a HUD scene through TgHUD vt[+0x430]); on the dedicated server
// myHUD is null so the native falls out at its Cast_TgHUD.
//
// Decompile: decompiled/TgGame/ATgOmegaVolume/ATgOmegaVolume__Used/
class TgOmegaVolume_Used : public HookBase<
	void(__fastcall*)(ATgOmegaVolume*, void*, ATgPlayerController*),
	0x109b98a0,
	TgOmegaVolume_Used> {
public:
	static void __fastcall Call(ATgOmegaVolume* Volume, void* edx, ATgPlayerController* UsingPlayer);
	static inline void __fastcall CallOriginal(ATgOmegaVolume* Volume, void* edx, ATgPlayerController* UsingPlayer) {
		m_original(Volume, edx, UsingPlayer);
	};
};
