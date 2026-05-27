#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__InitializeDefaultProps : public HookBase<
	void(__fastcall*)(ATgPawn*, void*),
	0x109BF400,
	TgPawn__InitializeDefaultProps> {
public:
	// SpawnBotById writes the spawning bot's id here right before the engine
	// invokes our hook (the native takes no args); we consume + clear it.
	static int nPendingBotId;

	static void __fastcall* Call(ATgPawn* Pawn, void* edx);
	static inline void __fastcall* CallOriginal(ATgPawn* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
};
