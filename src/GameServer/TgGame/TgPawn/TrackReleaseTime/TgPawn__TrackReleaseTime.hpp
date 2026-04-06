#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackReleaseTime : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, float),
	0x109c09d0,
	TgPawn__TrackReleaseTime> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fReleaseTime);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fReleaseTime) {
		m_original(Pawn, edx, nDeviceModeID, fReleaseTime);
	};
};
