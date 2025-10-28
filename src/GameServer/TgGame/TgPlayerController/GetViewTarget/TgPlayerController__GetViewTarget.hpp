#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__GetViewTarget : public HookBase<
	int(__fastcall*)(void*),
	0x10c77420,
	TgPlayerController__GetViewTarget> {
public:
	static int __fastcall Call(void* Controller);
	static inline int __fastcall CallOriginal(void* Controller) {
		return m_original(Controller);
	};
};


