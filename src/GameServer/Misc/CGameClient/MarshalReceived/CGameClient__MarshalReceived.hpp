#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CGameClient__MarshalReceived : public HookBase<
	uint8_t(__fastcall*)(void*, void*, void*),
	0x1091a7c0,
	CGameClient__MarshalReceived> {
public:
	static uint8_t __fastcall Call(void* GameClient, void* edx, void* InBunch);
	static inline uint8_t __fastcall CallOriginal(void* GameClient, void* edx, void* InBunch) {
		return m_original(GameClient, edx, InBunch);
	};
};

