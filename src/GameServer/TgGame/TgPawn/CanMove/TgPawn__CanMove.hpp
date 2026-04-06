#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__CanMove : public HookBase<
	bool(__fastcall*)(ATgPawn*, void*),
	0x109bf3b0,
	TgPawn__CanMove> {
public:
	static bool __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline bool __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		return m_original(Pawn, edx);
	};
};
