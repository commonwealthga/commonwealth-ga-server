#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackBuff : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, float),
	0x109c0940,
	TgPawn__TrackBuff> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fValue);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fValue) {
		m_original(Pawn, edx, nDeviceModeID, fValue);
	};
};
