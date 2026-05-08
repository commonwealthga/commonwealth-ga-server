#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__SetProperty : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int, float),
	0x109bf420,
	TgPawn__SetProperty> {
public:
	static __fastcall void Call(ATgPawn* Pawn, void* edx, int PropertyId, float NewValue);
	static inline __fastcall void CallOriginal(ATgPawn* Pawn, void* edx, int PropertyId, float NewValue) {
		m_original(Pawn, edx, PropertyId, NewValue);
	}
};
