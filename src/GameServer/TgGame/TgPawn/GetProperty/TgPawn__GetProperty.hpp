#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__GetProperty : public HookBase<
	UTgProperty*(__fastcall*)(ATgPawn*, void*, int),
	0x109dd2e0,
	TgPawn__GetProperty> {
public:
	static __fastcall UTgProperty* Call(ATgPawn* Pawn, void* edx, int PropertyId);
	static inline __fastcall UTgProperty* CallOriginal(ATgPawn* Pawn, void* edx, int PropertyId) {
		return m_original(Pawn, edx, PropertyId);
	}
};

