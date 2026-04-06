#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__GetDyeItemId : public HookBase<
	int(__fastcall*)(ATgPawn*, void*, unsigned char),
	0x109bfbe0,
	TgPawn__GetDyeItemId> {
public:
	static int __fastcall Call(ATgPawn* Pawn, void* edx, unsigned char eSlot);
	static inline int __fastcall CallOriginal(ATgPawn* Pawn, void* edx, unsigned char eSlot) {
		return m_original(Pawn, edx, eSlot);
	};
};
