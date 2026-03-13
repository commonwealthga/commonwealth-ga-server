#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__SetTaskForceNumber : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109bf910,
	TgPawn__SetTaskForceNumber> {
public:
	static void __fastcall* Call(ATgPawn* Pawn, void* edx, int TaskForceNumber);
	static inline void __fastcall* CallOriginal(ATgPawn* Pawn, void* edx, int TaskForceNumber) {
		m_original(Pawn, edx, TaskForceNumber);
	}
};


