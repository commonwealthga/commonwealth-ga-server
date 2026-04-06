#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerObama : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, int),
	0x109639f0,
	TgPlayerController__ServerObama> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nCurrency, int nCharId);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nCurrency, int nCharId) {
		m_original(Controller, edx, nCurrency, nCharId);
	}
};
