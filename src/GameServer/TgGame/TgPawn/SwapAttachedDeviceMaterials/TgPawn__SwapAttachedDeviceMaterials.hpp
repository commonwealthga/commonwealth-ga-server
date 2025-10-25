#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__SwapAttachedDeviceMaterials : public HookBase<
	void(__fastcall*)(int, void*, int, int),
	0x109c83a0,
	TgPawn__SwapAttachedDeviceMaterials> {
public:
	static void __fastcall Call(int Pawn, void* edx, int a2, int a3);
	static inline void __fastcall CallOriginal(int Pawn, void* edx, int a2, int a3) {
		m_original(Pawn, edx, a2, a3);
	};
};

