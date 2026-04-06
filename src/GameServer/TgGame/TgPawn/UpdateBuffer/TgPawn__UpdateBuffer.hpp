#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__UpdateBuffer : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgPawn*),
	0x109c0a30,
	TgPawn__UpdateBuffer> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgPawn* Buffer);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgPawn* Buffer) {
		m_original(Pawn, edx, Buffer);
	};
};
