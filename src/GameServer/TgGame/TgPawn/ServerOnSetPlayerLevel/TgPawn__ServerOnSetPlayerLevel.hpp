#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ServerOnSetPlayerLevel : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109c0330,
	TgPawn__ServerOnSetPlayerLevel> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nLevelId);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nLevelId) {
		m_original(Pawn, edx, nLevelId);
	};
};
