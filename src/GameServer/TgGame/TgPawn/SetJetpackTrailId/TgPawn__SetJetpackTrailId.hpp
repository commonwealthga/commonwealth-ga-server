#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__SetJetpackTrailId : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109bfbc0,
	TgPawn__SetJetpackTrailId> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nId);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nId) {
		m_original(Pawn, edx, nId);
	};
};
