#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerLoadItemProfile : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int),
	0x10963050,
	TgPlayerController__ServerLoadItemProfile> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nId);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nId) {
		m_original(Controller, edx, nId);
	}
};
