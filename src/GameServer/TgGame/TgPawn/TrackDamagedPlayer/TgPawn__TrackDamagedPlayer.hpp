#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackDamagedPlayer : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgPawn*, int, int),
	0x109c0880,
	TgPawn__TrackDamagedPlayer> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgPawn* TargetPawn, int nDeviceModeID, int nDamage);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgPawn* TargetPawn, int nDeviceModeID, int nDamage) {
		m_original(Pawn, edx, TargetPawn, nDeviceModeID, nDamage);
	};
};
