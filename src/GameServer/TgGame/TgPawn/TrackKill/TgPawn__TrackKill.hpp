#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackKill : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgPawn*),
	0x109c0a50,
	TgPawn__TrackKill> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgPawn* Killer);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgPawn* Killer) {
		m_original(Pawn, edx, Killer);
	};
};
