#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerAddHZPoints : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, int),
	0x10963a10,
	TgPlayerController__ServerAddHZPoints> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nValue, int nCharId);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nValue, int nCharId) {
		m_original(Controller, edx, nValue, nCharId);
	};
};
