#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__SetSoundMode : public HookBase<
	void(__stdcall*)(int, int),
	0x10963c10,
	TgPlayerController__SetSoundMode> {
public:
	static void __stdcall Call(int Controller, int NewSoundMode);
	static inline void __stdcall CallOriginal(int Controller, int NewSoundMode) {
		m_original(Controller, NewSoundMode);
	};
};


