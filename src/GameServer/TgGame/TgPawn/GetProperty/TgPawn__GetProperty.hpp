#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__GetProperty : public HookBase<
	UTgProperty*(__fastcall*)(ATgPawn*, void*, int),
	0x10AD9B80,
	TgPawn__GetProperty> {
public:
	static UTgProperty* Call(ATgPawn* Pawn, void* edx, int PropertyId);
	static inline UTgProperty* CallOriginal(ATgPawn* Pawn, void* edx, int PropertyId) {
		return m_original(Pawn, edx, PropertyId);
	}
};

