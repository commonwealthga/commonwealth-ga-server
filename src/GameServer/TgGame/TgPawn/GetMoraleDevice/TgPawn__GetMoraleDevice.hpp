#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__GetMoraleDevice : public HookBase<
	ATgDevice_Morale*(__fastcall*)(ATgPawn*, void*),
	0x109bf7c0,
	TgPawn__GetMoraleDevice> {
public:
	static ATgDevice_Morale* __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline ATgDevice_Morale* __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		return m_original(Pawn, edx);
	};
};
