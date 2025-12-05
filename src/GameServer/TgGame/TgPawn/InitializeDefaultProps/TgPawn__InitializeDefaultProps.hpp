#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__InitializeDefaultProps : public HookBase<
	void(__fastcall*)(ATgPawn*, void*),
	0x109BF400,
	TgPawn__InitializeDefaultProps> {
public:
	static void __fastcall* Call(ATgPawn* Pawn, void* edx);
	static inline void __fastcall* CallOriginal(ATgPawn* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
	static UTgProperty* InitializeProperty(ATgPawn* Pawn, int nPropertyId, float fBase, float fRaw, float fMinimum, float fMaximum);
};

