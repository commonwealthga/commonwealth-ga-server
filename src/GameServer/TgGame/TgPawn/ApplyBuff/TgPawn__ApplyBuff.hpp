#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ApplyBuff : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, FBuffHeader, int, float, unsigned long, unsigned char),
	0x109bf7b0,
	TgPawn__ApplyBuff> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, FBuffHeader BuffFilter, int nCalcMethodCode, float fAmount, unsigned long bRemove, unsigned char buffSourceType);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, FBuffHeader BuffFilter, int nCalcMethodCode, float fAmount, unsigned long bRemove, unsigned char buffSourceType) {
		m_original(Pawn, edx, BuffFilter, nCalcMethodCode, fAmount, bRemove, buffSourceType);
	};
};
