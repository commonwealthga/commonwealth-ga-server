#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackObjectivePoints : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109c0970,
	TgPawn__TrackObjectivePoints> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nPoints);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nPoints) {
		m_original(Pawn, edx, nPoints);
	};
};
