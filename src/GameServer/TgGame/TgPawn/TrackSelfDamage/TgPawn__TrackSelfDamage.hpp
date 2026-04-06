#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackSelfDamage : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, int),
	0x109c09c0,
	TgPawn__TrackSelfDamage> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage) {
		m_original(Pawn, edx, nDeviceModeID, nDamage);
	};
};
