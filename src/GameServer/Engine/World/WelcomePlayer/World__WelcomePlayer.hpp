#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class World__WelcomePlayer : public HookBase<
	void(__fastcall*)(UWorld*, void*, UNetConnection*, wchar_t*),
	0x10BEFE20,
	World__WelcomePlayer> {
public:
	static void __fastcall Call(UWorld* World, void* edx, UNetConnection* Connection, wchar_t* ExtraOptions);
	static inline void __fastcall CallOriginal(UWorld* World, void* edx, UNetConnection* Connection, wchar_t* ExtraOptions) {
		LogCallOriginalBegin();
		m_original(World, edx, Connection, ExtraOptions);
		LogCallOriginalEnd();
	}
};

