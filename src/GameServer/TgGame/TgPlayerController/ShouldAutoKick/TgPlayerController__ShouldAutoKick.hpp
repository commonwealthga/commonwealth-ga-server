#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ShouldAutoKick : public HookBase<
	bool(__fastcall*)(ATgPlayerController*, void*),
	0x10963540,
	TgPlayerController__ShouldAutoKick> {
public:
	static bool __fastcall Call(ATgPlayerController* Controller, void* edx);
	static inline bool __fastcall CallOriginal(ATgPlayerController* Controller, void* edx) {
		return m_original(Controller, edx);
	}
};
