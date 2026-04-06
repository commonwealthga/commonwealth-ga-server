#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerClearProfiles : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*),
	0x109630a0,
	TgPlayerController__ServerClearProfiles> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx) {
		m_original(Controller, edx);
	};
};
