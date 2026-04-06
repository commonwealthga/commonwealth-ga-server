#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__UpdateControllerVisBasedProperties : public HookBase<
	void(__fastcall*)(ATgPawn*, void*),
	0x109c0db0,
	TgPawn__UpdateControllerVisBasedProperties> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
};
