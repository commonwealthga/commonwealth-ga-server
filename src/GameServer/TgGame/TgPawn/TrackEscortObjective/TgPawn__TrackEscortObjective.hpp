#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackEscortObjective : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, float),
	0x109c0950,
	TgPawn__TrackEscortObjective> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, float fDeltaTime);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, float fDeltaTime) {
		m_original(Pawn, edx, fDeltaTime);
	};
};
