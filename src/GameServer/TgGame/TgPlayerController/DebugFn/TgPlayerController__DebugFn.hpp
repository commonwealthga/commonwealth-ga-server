#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__DebugFn : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, unsigned long, unsigned long),
	0x109638d0,
	TgPlayerController__DebugFn> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, unsigned long val, unsigned long val2);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, unsigned long val, unsigned long val2) {
		m_original(Controller, edx, val, val2);
	};
};
