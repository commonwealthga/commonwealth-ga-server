#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackHit : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, float, unsigned long),
	0x109c08f0,
	TgPawn__TrackHit> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDistance, unsigned long bHitPlayer);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDistance, unsigned long bHitPlayer) {
		m_original(Pawn, edx, nDeviceModeID, fDistance, bHitPlayer);
	};
};
