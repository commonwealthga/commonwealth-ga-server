#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__IsReadyForStart : public HookBase<
	int(__fastcall*)(void),
	0x10962FA0,
	TgPlayerController__IsReadyForStart> {
public:
	static int __fastcall Call(void);
	static inline int __fastcall CallOriginal(void) {
		return m_original();
	}
};

