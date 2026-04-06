#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackTeamDamage : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, int),
	0x109c09a0,
	TgPawn__TrackTeamDamage> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage) {
		m_original(Pawn, edx, nDeviceModeID, nDamage);
	};
};
