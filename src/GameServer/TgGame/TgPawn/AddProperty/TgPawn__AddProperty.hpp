#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__AddProperty : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, float, float, float, float),
	0x109bf410,
	TgPawn__AddProperty> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nPropId, float fBase, float fRaw, float FMin, float FMax);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nPropId, float fBase, float fRaw, float FMin, float FMax) {
		m_original(Pawn, edx, nPropId, fBase, fRaw, FMin, FMax);
	};
};
