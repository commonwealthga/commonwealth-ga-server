#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackMyBeaconUsed : public HookBase<
	void(__fastcall*)(ATgPawn*, void*),
	0x109c09e0,
	TgPawn__TrackMyBeaconUsed> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
};
