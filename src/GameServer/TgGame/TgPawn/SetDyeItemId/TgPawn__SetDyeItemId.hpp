#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__SetDyeItemId : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, unsigned char),
	0x109bfbf0,
	TgPawn__SetDyeItemId> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nId, unsigned char eSlot);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nId, unsigned char eSlot) {
		m_original(Pawn, edx, nId, eSlot);
	};
};
